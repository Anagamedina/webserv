#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <sstream>

#include "Client.hpp"
#include "ErrorUtils.hpp"
#include "cgi/CgiExecutor.hpp"
#include "cgi/CgiProcess.hpp"
#include "http/HttpHeaderUtils.hpp"
#include "network/ServerManager.hpp"

void Client::setServerManager(ServerManager* serverManager) {
  _serverManager = serverManager;
}

static void parseCgiHeaders(const std::string& headers,
                            HttpResponse& response) {
  std::istringstream iss(headers);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty() && line[line.length() - 1] == '\r')
      line.erase(line.length() - 1);
    if (line.empty()) continue;

    std::string key;
    std::string value;
    if (!http_header_utils::splitHeaderLine(line, key, value)) continue;

    std::string keyLower = key;
    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
                   ::tolower);
    if (keyLower == "status") continue;
    if (keyLower == "connection") continue;
    if (keyLower == "transfer-encoding") continue;
    response.setHeader(key, value);
  }
}

bool Client::executeCgi(const RequestProcessor::CgiInfo& cgiInfo) {
  if (_serverManager == 0 || cgiInfo.server == 0) return false;

  // No need to validate location or method here, RequestProcessor did it.

  CgiExecutor exec;
  const HttpRequest& request = _parser.getRequest();

  _cgiProcess = exec.executeAsync(request, cgiInfo.scriptPath,
                                  cgiInfo.interpreterPath, *cgiInfo.server);

  if (_cgiProcess == 0) {
    // If execution fails (e.g. pipe error), return false so caller can send 500
    return false;
  }

  _serverManager->registerCgiPipe(_cgiProcess->getPipeOut(),
                                  EPOLLIN | EPOLLRDHUP, this);
  _serverManager->registerCgiPipe(_cgiProcess->getPipeIn(),
                                  EPOLLOUT | EPOLLRDHUP, this);

  _state = STATE_READING_BODY;

  // Save request state needed for finalization
  _savedShouldClose = request.shouldCloseConnection();
  _savedVersion = request.getVersion();
  _cgiServerConfig = cgiInfo.server;

  return true;
}

/**
 * Finalizes the HTTP response from CGI script output.
 *
 * This function parses the CGI output buffer to extract HTTP headers and body,
 * validates the response structure, and constructs a complete HttpResponse
 * object.
 *
 * Process:
 * 1. Parse CGI headers (Status, Content-Type, Location, etc.)
 * 2. Determine HTTP status code (default 200 if not specified)
 * 3. Validate response coherence (e.g., redirects must include Location header)
 * 4. Set default Content-Type if missing
 * 5. Populate HttpResponse with parsed headers and body
 *
 * @param request The original HTTP request
 * @param cgi_output Raw output buffer from the CGI process
 * @param response HttpResponse object to populate
 * @param server Server configuration for error page handling
 *
 * @return true if response was successfully finalized
 *         false if CGI output was invalid (triggers 500 error response)
 *
 * @note Handles both document responses (200 OK) and local redirects (3xx)
 *       as per CGI/1.1 specification (RFC 3875)
 */
void Client::finalizeCgiResponse(const CgiProcess* finishedProcess) {
  if (finishedProcess == 0) {
    return;
  }

  int child_status = 0;
  bool has_child_status = false;
  if (_serverManager != 0) {
    has_child_status = _serverManager->consumeCgiExitStatus(
        finishedProcess->getPid(), child_status);
  }

  if (has_child_status) {
    if ((WIFEXITED(child_status) && WEXITSTATUS(child_status) != 0) ||
        WIFSIGNALED(child_status)) {
      _response.clear();
      buildErrorResponse(_response, _parser.getRequest(), 500, true, _cgiServerConfig);

      enqueueResponse(_response.serialize(), true);
      processRequests();
      return;
    }
  }

  _response.setStatusCode(finishedProcess->getStatusCode());
  if (_savedVersion == HTTP_VERSION_1_0)
    _response.setVersion("HTTP/1.0");
  else
    _response.setVersion("HTTP/1.1");
  _response.setHeader("Connection", _savedShouldClose ? "close" : "keep-alive");

  if (finishedProcess->isHeadersComplete()) {
    parseCgiHeaders(finishedProcess->getResponseHeaders(), _response);
    _response.setBody(finishedProcess->getResponseBody());
  } else {
    if (!_response.hasHeader("Content-Type")) {
      _response.setHeader("Content-Type", "text/plain");
    }
    _response.setBody(finishedProcess->getCompleteResponse());
  }

  std::vector<char> serialized = _response.serialize();
  enqueueResponse(serialized, _savedShouldClose);

  // Resume processing requests (in case pipelined data is waiting)
  processRequests();
}

void Client::handleCgiPipe(int pipe_fd, size_t events) {
  if (_cgiProcess == 0) {
    if (_serverManager) {
      _serverManager->unregisterCgiPipe(pipe_fd);
    }
    return;
  }

  if (pipe_fd == _cgiProcess->getPipeIn()) {
    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
      _serverManager->unregisterCgiPipe(pipe_fd);
      _cgiProcess->closePipeIn();
      return;
    }

    if (!(events & EPOLLOUT)) {
      return;
    }

    const std::string& body = _cgiProcess->getRequestBody();
    size_t offset = _cgiProcess->getBodyBytesWritten();
    if (offset < body.size()) {
      ssize_t written =
          write(pipe_fd, body.c_str() + offset, body.size() - offset);
      if (written > 0) {
        _cgiProcess->advanceBodyBytesWritten(static_cast<size_t>(written));
        _lastActivity = std::time(0);
      } else if (written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          return;
        }

        // Si el CGI cerró su extremo de lectura (EPIPE), o hay otro error,
        // simplemente dejamos de intentar escribirle. No cortamos la respuesta,
        // porque el CGI puede ya haber producido salida antes de salir, o
        // simplemente no quería leer el body entero.
        _serverManager->unregisterCgiPipe(pipe_fd);
        _cgiProcess->closePipeIn();
        return;
      }
    }
    if (_cgiProcess->isRequestBodySent()) {
      _serverManager->unregisterCgiPipe(pipe_fd);
      _cgiProcess->closePipeIn();
    }
    return;
  }

  if (pipe_fd == _cgiProcess->getPipeOut() &&
      (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP))) {
    char buffer[4096];
    ssize_t bytes = read(pipe_fd, buffer, sizeof(buffer));
    if (bytes > 0) {
      _cgiProcess->appendResponseData(buffer, static_cast<size_t>(bytes));
      _lastActivity = std::time(0);
      return;
    }
    if (bytes == 0) {
      // EOF - Pipe closed by CGI process
      CgiProcess* finished = _cgiProcess;
      _serverManager->unregisterCgiPipe(pipe_fd);
      int pipeIn = finished->getPipeIn();
      if (pipeIn >= 0) {
        _serverManager->unregisterCgiPipe(pipeIn);
        finished->closePipeIn();
      }
      finished->closePipeOut();
      _cgiProcess = 0;
      finalizeCgiResponse(finished);
      delete finished;
      return;
    }
    if (bytes < 0) {
      // Check for non-blocking I/O errors
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available right now, try again later
        return;
      }
      // Real pipe read error — CGI output is unreliable.
      // Build a clean 502 Bad Gateway instead of forwarding partial data.
      _serverManager->unregisterCgiPipe(pipe_fd);
      int pipeIn = _cgiProcess->getPipeIn();
      if (pipeIn >= 0) {
        _serverManager->unregisterCgiPipe(pipeIn);
        _cgiProcess->closePipeIn();
      }
      _cgiProcess->closePipeOut();
      _cgiProcess->terminateProcess();
      delete _cgiProcess;
      _cgiProcess = 0;

      _response.clear();
      buildErrorResponse(_response, _parser.getRequest(), 502, true, _cgiServerConfig);
      enqueueResponse(_response.serialize(), true);
      return;
    }
  }
}

bool Client::checkCgiTimeout() {
  if (_cgiProcess == 0) {
    return false;
  }

  if (!_cgiProcess->isTimedOut()) {
    return false;
  }

  int pipeIn = _cgiProcess->getPipeIn();
  int pipeOut = _cgiProcess->getPipeOut();

  if (_serverManager) {
    if (pipeIn >= 0) {
      _serverManager->unregisterCgiPipe(pipeIn);
    }
    if (pipeOut >= 0) {
      _serverManager->unregisterCgiPipe(pipeOut);
    }
  }

  _cgiProcess->closePipeIn();
  _cgiProcess->closePipeOut();
  _cgiProcess->terminateProcess();
  delete _cgiProcess;
  _cgiProcess = 0;

  _response.clear();
  buildErrorResponse(_response, _parser.getRequest(), 504, true, _cgiServerConfig);

  enqueueResponse(_response.serialize(), true);
  _lastActivity = std::time(0);
  return true;
}
