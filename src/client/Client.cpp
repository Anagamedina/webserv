#include "Client.hpp"
#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

// =============================================================================
// FUNCIONES AUXILIARES (solo usadas dentro de la clase)
// =============================================================================

void Client::handleExpect100() {
  // Expect: 100-continue: el cliente espera confirmación antes de mandar body grande
  if (_parser.getState() == PARSING_BODY &&
      _parser.getRequest().hasExpect100Continue() && !_sent100Continue) {
    std::string continueMsg("HTTP/1.1 100 Continue\r\n\r\n");
    enqueueResponse(
        std::vector<char>(continueMsg.begin(), continueMsg.end()), false);
    _sent100Continue = true;
  }
}

void Client::enqueueResponse(const std::vector<char>& data, bool closeAfter) {
  // Añade una respuesta a la cola. Si no hay nada enviando, la pone en _outBuffer.
  std::string payload(data.begin(), data.end());
  if (_outBuffer.empty()) {
    _outBuffer = payload;
    _closeAfterWrite = closeAfter;
    _state = STATE_WRITING_RESPONSE;
    return;
  }
  _responseQueue.push(PendingResponse(payload, closeAfter));
}

void Client::buildResponse() {
  const HttpRequest& request = _parser.getRequest();
  // TODO: this must change, starting cgi will be inside process
  if (startCgiIfNeeded(request)) return;
  _processor.process(request, _configs, _listenPort,
                     _parser.getErrorStatusCode(), _response);
}

bool Client::handleCompleteRequest() {
  // Se llama cuando el parser tiene una request completa (o con error)
  const HttpRequest& request = _parser.getRequest();
  bool shouldClose =
      (_parser.getState() == ERROR) || request.shouldCloseConnection();

#ifdef DEBUG
  std::cerr << "[CLIENT] Request complete: " << request.getMethod()
            << " " << request.getPath()
            << " (body size: " << request.getBody().size() << " bytes";
  if (request.getBody().size() > 1024 * 1024) {
    std::cerr << " = " << (request.getBody().size() / 1024 / 1024) << " MB";
  }
  std::cerr << ")" << std::endl;
#endif

  // If parser is in ERROR state, we should generate error response immediately
  // and NOT try to run CGI (which requires valid body/headers)
  if (_parser.getState() == ERROR) {
      _processor.process(request, _configs, _listenPort,
                         _parser.getErrorStatusCode(), _response);
  } else {
      buildResponse();
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

Client::Client(int fd, const std::vector<ServerConfig>* configs, int listenPort)
    : _savedShouldClose(false),
      _savedVersion(HTTP_VERSION_1_1),
      _fd(fd),
      _listenPort(listenPort),
      _configs(configs),
      _state(STATE_IDLE),
      _lastActivity(std::time(0)),
      _outBuffer(),
      _responseQueue(),
      _parser(),
      _response(),
      _serverManager(0),
      _cgiProcess(0),
      _closeAfterWrite(false),
      _sent100Continue(false) {
  const ServerConfig* server = selectServerByPort(listenPort, configs);
  if (server) _parser.setMaxBodySize(server->getGlobalMaxBodySize());
}

Client::~Client() {}

int Client::getFd() const { return _fd; }

ClientState Client::getState() const { return _state; }

bool Client::needsWrite() const { return !_outBuffer.empty(); }

bool Client::hasPendingData() const {
  return !_outBuffer.empty() || !_responseQueue.empty();
}

time_t Client::getLastActivity() const { return _lastActivity; }

// =============================================================================
// MANEJO DE EVENTOS (llamados desde el bucle epoll)
// =============================================================================

void Client::handleRead() {
  // 1) Leer datos del socket
  char buffer[4096];
  ssize_t bytesRead = recv(_fd, buffer, sizeof(buffer), 0);

  if (bytesRead > 0) {
    _lastActivity = std::time(0);
    if (_state == STATE_IDLE) _state = STATE_READING_HEADER;

    // 2) Pasar al parser
    _parser.consume(std::string(buffer, bytesRead));

    // 3) Expect: 100-continue (respuesta intermedia si el cliente la espera)
    handleExpect100();

    processRequests();

    // 5) Si el parser marcó error, construir y enviar respuesta de error
    if (_parser.getState() == ERROR) {
      handleCompleteRequest();
      return;
    }
  } else if (bytesRead == 0) {
    _state = STATE_CLOSED;  // Cliente cerró la conexión
  } else {
    _state = STATE_CLOSED;  // Error en recv
  }
}

void Client::processRequests() {
  while (_parser.getState() == COMPLETE) {
    // If a CGI process is running, we cannot start another one or process
    // responses yet. We just wait (parser buffer holds next request).
    if (_cgiProcess) return;

    bool shouldClose = handleCompleteRequest();

    // If CGI started, handleCompleteRequest returned true (and set _cgiProcess).
    // The parser holds the request that started the CGI. We must reset it
    // so we can parse the *next* request (if any) later.
    // BUT we must have saved the necessary info from the request first
    // (done in startCgiIfNeeded).
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
// - Intenta enviar parte de _outBuffer con send().
// - Borra del buffer lo que se haya enviado.
// - Si termina y hay mas respuestas en cola, las saca una a una.
// - Si no hay nada mas y no hay que cerrar, vuelve a STATE_IDLE.

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

  // Si hemos enviado todo el buffer actual:
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
