#include "Client.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"
#include "cgi/CgiProcess.hpp"
#include "network/ServerManager.hpp"



/**
 * @brief Handle Expect: 100-continue
 * 
 * If the parser is in the PARSING_BODY state and the request has an Expect: 100-continue header,
 * and the sent100Continue flag is not set, then enqueue a response with the continue message.
 * The sent100Continue flag is set to true.
 * 
 *
 */
void Client::handleExpect100() {
  if (_parser.getState() == PARSING_BODY &&
      _parser.getRequest().hasExpect100Continue() && !_sent100Continue) {
    std::string continueMsg("HTTP/1.1 100 Continue\r\n\r\n");
    enqueueResponse(std::vector<char>(continueMsg.begin(), continueMsg.end()),
                    false);
    _sent100Continue = true;
  }
}


/*
 * @brief Enqueue a response with the continue message.
 * 
 * @param data The data to enqueue.
 * @param closeAfter Whether to close the connection after writing the response.
 * 
 */
void Client::enqueueResponse(const std::vector<char>& data, bool closeAfter) {
  std::string payload(data.begin(), data.end());
  if (_outBuffer.empty()) {
    _outBuffer = payload;
    _closeAfterWrite = closeAfter;
    _state = STATE_WRITING_RESPONSE;
    return;
  }
  _responseQueue.push(PendingResponse(payload, closeAfter));
}


bool Client::startCgi(const RequestProcessor::CgiInfo& cgiInfo) {
  return executeCgi(cgiInfo);
}

/*
 * @brief Dispatch an action based on the processor result.
 * 
 * Dispatch an action based on the processor result.
 * If the action is ACTION_EXECUTE_CGI, start a CGI process.
 * If the action is ACTION_SEND_RESPONSE, send a response.
 * 
 */
void Client:: dispatchAction(const HttpRequest& request,
                            const RequestProcessor::ProcessingResult& result) {
  if (result.action == RequestProcessor::ACTION_EXECUTE_CGI) {
    if (startCgi(result.cgiInfo)) {
      return;
    }
    const ServerConfig* server = selectServerByPort(_listenPort, _configs);
    buildErrorResponse(_response, request, 500, true, server);
    _forceCloseCurrentResponse = true;
    return;
  }

  _response = result.response;
}


/*
 * @brief Build a response.
 * 
 * Build a response based on the request and the processor result.
 * forceCloseCurrentResponse is set to false because the response is not yet built.
 */
void Client::buildResponse() {
  const HttpRequest& request = _parser.getRequest();
  _forceCloseCurrentResponse = false;

  RequestProcessor::ProcessingResult result = _processor.process(
      request, _configs, _listenPort, _parser.getErrorStatusCode());

  dispatchAction(request, result);
}


/*
 * @brief Handle a complete request.
 * 
 * Handle a complete request.
 * If the parser is in the ERROR state, handle the complete request.
 * If the parser is in the COMPLETE state, handle the complete request.
 * serialize the response and enqueue it.
 * return true if the connection should be closed.
 * forceCloseCurrentResponse is set to false because the response is not yet built.
 */
bool Client::handleCompleteRequest() {
  const HttpRequest& request = _parser.getRequest();
  bool shouldClose =
      (_parser.getState() == ERROR) || request.shouldCloseConnection();

#ifdef DEBUG
  std::cerr << "[CLIENT] Request complete: " << request.getMethod() << " "
            << request.getPath() << " (body size: " << request.getBody().size()
            << " bytes";
  if (request.getBody().size() > 1024 * 1024) {
    std::cerr << " = " << (request.getBody().size() / 1024 / 1024) << " MB";
  }
  std::cerr << ")" << std::endl;
#endif

  if (_parser.getState() == ERROR) {
    RequestProcessor::ProcessingResult result = _processor.process(
        request, _configs, _listenPort, _parser.getErrorStatusCode());
    _response = result.response;
  } else {
    buildResponse();
    if (_forceCloseCurrentResponse) {
      shouldClose = true;
    }
    if (_cgiProcess) {
      return true;
    }
  }
  std::vector<char> serialized = _response.serialize();
  enqueueResponse(serialized, shouldClose);
  return shouldClose;
}

// =============================================================================
// CONSTRUCTOR, DESTRUCTOR, GETTERS
// =============================================================================

Client::Client(int fd, const std::vector<ServerConfig>* configs, int listenPort, const std::string& clientIp)
    : _savedShouldClose(false),
      _savedVersion(HTTP_VERSION_1_1),
      _fd(fd),
      _listenPort(listenPort),
      _clientIp(clientIp),
      _configs(configs),
      _state(STATE_IDLE),
      _lastActivity(std::time(0)),
      _forceCloseCurrentResponse(false),
      _outBuffer(),
      _responseQueue(),
      _parser(),
      _response(),
      _serverManager(0),
      _cgiProcess(0),
      _cgiServerConfig(0) {
  const ServerConfig* server = selectServerByPort(listenPort, configs);
  if (server) _parser.setMaxBodySize(server->getGlobalMaxBodySize());
}

Client::~Client() {
  if (_cgiProcess) {
    int pipeIn = _cgiProcess->getPipeIn();
    int pipeOut = _cgiProcess->getPipeOut();

    if (_serverManager) {
      if (pipeIn >= 0) _serverManager->unregisterCgiPipe(pipeIn);
      if (pipeOut >= 0) _serverManager->unregisterCgiPipe(pipeOut);
    }

    _cgiProcess->closePipeIn();
    _cgiProcess->closePipeOut();
    _cgiProcess->terminateProcess();
    delete _cgiProcess;
    _cgiProcess = 0;
  }

  if (_fd >= 0) {
    close(_fd);
    _fd = -1;
  }
}

int Client::getFd() const { return _fd; }

ClientState Client::getState() const { return _state; }

bool Client::needsWrite() const { return !_outBuffer.empty(); }

bool Client::hasPendingData() const {
  return _cgiProcess != 0 || !_outBuffer.empty() || !_responseQueue.empty();
}

time_t Client::getLastActivity() const { return _lastActivity; }

// =============================================================================
// MANEJO DE EVENTOS (llamados desde el bucle epoll)
// =============================================================================

/*
 * @brief Handle a read event.
 * 
 * Read data from the socket and update the parser.
 * Handle Expect: 100-continue.
 * Process requests.
 * If the parser is in the ERROR state, handle the complete request.
 * 
 */
void Client::handleRead() {
  char buffer[4096];
  ssize_t bytesRead = 0;

  bytesRead = recv(_fd, buffer, sizeof(buffer), 0);
  if (bytesRead > 0) {
    _lastActivity = std::time(0);
    if (_state == STATE_IDLE) _state = STATE_READING_HEADER;

    _parser.consume(std::string(buffer, bytesRead));
    handleExpect100();
    processRequests();

    if (_parser.getState() == ERROR) {
      handleCompleteRequest();
      return;
    }
  } else if (bytesRead == 0) {
    _state = STATE_CLOSED;
  } else {
    _state = STATE_CLOSED;
  }
}

/*
 * @brief Process requests.
 * 
 * Process requests until the parser is not in the COMPLETE state.
 * If the parser is in the COMPLETE state, handle the complete request.
 * 
 */
void Client::processRequests() {
  while (_parser.getState() == COMPLETE) {
    if (_cgiProcess) return;
    bool shouldClose = handleCompleteRequest();
    if (_cgiProcess) {
      _response.clear();
      _parser.reset();
      return;
    }

    if (shouldClose) return;
    _response.clear();
    _parser.reset();
    _sent100Continue = false;
    _parser.consume("");
  }
}

// ============================
// ESCRITURA AL SOCKET (EPOLLOUT)
// ============================

/*
 * @brief Handle a write event.
 * 
 * Send data to the socket.
 * If the out buffer is empty, return.
 * If the out buffer is not empty, send the data to the socket.
 * 
 */
void Client::handleWrite() {
  if (_outBuffer.empty()) return;

  ssize_t bytesSent = send(_fd, _outBuffer.c_str(), _outBuffer.size(), 0);
  if (bytesSent > 0) {
    _lastActivity = std::time(0);
    _outBuffer.erase(0, bytesSent);
  } else if (bytesSent < 0) {
    _state = STATE_CLOSED;
    return;
  }

  if (_outBuffer.empty()) {
    if (_closeAfterWrite == true) {
      _state = STATE_CLOSED;
      return;
    }
    if (_responseQueue.empty() == false) {
      PendingResponse next = _responseQueue.front();
      _responseQueue.pop();
      _outBuffer = next.data;
      _closeAfterWrite = next.closeAfter;
      _state = STATE_WRITING_RESPONSE;
      return;
    }
    _state = STATE_IDLE;
  }
}
