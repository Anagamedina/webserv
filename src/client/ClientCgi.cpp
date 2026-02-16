#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <sstream>

#include "Client.hpp"
#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"
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
    response.setHeader(key, value);
  }
}

bool Client::executeCgi(const RequestProcessor::CgiInfo& cgiInfo) {
  if (_configs == 0 || _serverManager == 0) return false;

  const ServerConfig* server = selectServerByPort(_listenPort, _configs);
  if (server == 0) return false;

  // No need to validate location or method here, RequestProcessor did it.
  
  CgiExecutor exec;
  const HttpRequest& request = _parser.getRequest();
  
  _cgiProcess = exec.executeAsync(request, cgiInfo.scriptPath, cgiInfo.interpreterPath, *server);
  
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

  return true;
}

void Client::finalizeCgiResponse() {
  _response.setStatusCode(_cgiProcess->getStatusCode());
  if (_savedVersion == HTTP_VERSION_1_0)
    _response.setVersion("HTTP/1.0");
  else
    _response.setVersion("HTTP/1.1");
  _response.setHeader("Connection", _savedShouldClose ? "close" : "keep-alive");

  parseCgiHeaders(_cgiProcess->getResponseHeaders(), _response);
  _response.setBody(_cgiProcess->getResponseBody());

  std::vector<char> serialized = _response.serialize();
  enqueueResponse(serialized, _savedShouldClose);

  // Resume processing requests (in case pipelined data is waiting)
  processRequests();
}

void Client::handleCgiPipe(int pipe_fd, size_t events) {
  if (_cgiProcess == 0) return;

  if (pipe_fd == _cgiProcess->getPipeIn() && (events & EPOLLOUT)) {
    const std::string& body = _cgiProcess->getRequestBody();
    size_t offset = _cgiProcess->getBodyBytesWritten();
    if (offset < body.size()) {
      ssize_t written =
          write(pipe_fd, body.c_str() + offset, body.size() - offset);
      if (written > 0) {
        _cgiProcess->advanceBodyBytesWritten(static_cast<size_t>(written));
        _lastActivity = std::time(0);
      }
      // Note: if written < 0 with EAGAIN/EWOULDBLOCK, we'll retry on next
      // EPOLLOUT This is correct for non-blocking I/O - offset not advanced,
      // will retry
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
      _serverManager->unregisterCgiPipe(pipe_fd);
      _cgiProcess->closePipeOut();
      finalizeCgiResponse();
      delete _cgiProcess;
      _cgiProcess = 0;
      return;
    }
    if (bytes < 0) {
      // Check for non-blocking I/O errors
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available right now, try again later
        return;
      }
      // Real error occurred
      _serverManager->unregisterCgiPipe(pipe_fd);
      _cgiProcess->closePipeOut();
      _cgiProcess->terminateProcess();
      // TODO: Maybe set error 500 in response?
      finalizeCgiResponse();
      delete _cgiProcess;
      _cgiProcess = 0;
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
  _response.setStatusCode(HTTP_STATUS_INTERNAL_SERVER_ERROR);
  if (_savedVersion == HTTP_VERSION_1_0)
    _response.setVersion("HTTP/1.0");
  else
    _response.setVersion("HTTP/1.1");
  _response.setHeader("Connection", "close");
  _response.setHeader("Content-Type", "text/plain");
  _response.setBody("CGI timeout\n");

  enqueueResponse(_response.serialize(), true);
  _lastActivity = std::time(0);
  return true;
}
