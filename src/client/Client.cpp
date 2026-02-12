#include "Client.hpp"
#include "RequestProcessorUtils.hpp"

#include <sys/socket.h>
#include <unistd.h>

// ============================
// FUNCIONES AUXILIARES
// ============================

void Client::handleExpect100() {
  if (_parser.getState() == PARSING_BODY &&
      _parser.getRequest().hasExpect100Continue() && !_sent100Continue) {
    std::string continueMsg("HTTP/1.1 100 Continue\r\n\r\n");
    enqueueResponse(
        std::vector<char>(continueMsg.begin(), continueMsg.end()), false);
    _sent100Continue = true;
  }
}

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

void Client::buildResponse() {
  const HttpRequest& request = _parser.getRequest();
  if (startCgiIfNeeded(request)) return;
  _processor.process(request, _configs, _listenPort,
                     _parser.getErrorStatusCode(), _response);
}

bool Client::handleCompleteRequest() {
  const HttpRequest& request = _parser.getRequest();
  bool shouldClose =
      (_parser.getState() == ERROR) || request.shouldCloseConnection();
  buildResponse();
  if (_cgiProcess) {
    return true;
  }
  std::vector<char> serialized = _response.serialize();
  enqueueResponse(serialized, shouldClose);
  return shouldClose;
}

// ============================
// CONSTRUCTOR, DESTRUCTOR, GETTERS
// ============================

Client::Client(int fd, const std::vector<ServerConfig>* configs, int listenPort)
    : _fd(fd),
      _inBuffer(),
      _outBuffer(),
      _parser(),
      _response(),
      _processor(),
      _configs(configs),
      _listenPort(listenPort),
      _state(STATE_IDLE),
      _lastActivity(std::time(0)),
      _serverManager(0),
      _cgiProcess(0),

      _closeAfterWrite(false),
      _sent100Continue(false),
      _responseQueue(),
      _savedShouldClose(false),
      _savedVersion(HTTP_VERSION_1_1) {
  const ServerConfig* server = selectServerByPort(listenPort, configs);
  if (server) _parser.setMaxBodySize(server->getMaxBodySize());
}

Client::~Client() {}

int Client::getFd() const { return _fd; }

ClientState Client::getState() const { return _state; }

bool Client::needsWrite() const { return !_outBuffer.empty(); }

bool Client::hasPendingData() const {
  return !_outBuffer.empty() || !_responseQueue.empty();
}

time_t Client::getLastActivity() const { return _lastActivity; }

// ============================
// MANEJO DE EVENTOS (EPOLL)
// ============================

void Client::handleRead() {
  char buffer[4096];  // buffer temporal
  ssize_t bytesRead = 0;

  // Pasos:
  // 1) Recibir datos crudos.
  // 2) Actualizar tiempo para evitar timeout.
  // 3) Pasar los datos al parser HTTP.
  // 4) Ver si el parser ha completado una o mas requests.
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

  // 1) Intentar enviar lo que queda en el buffer.
  // 2) Si hemos terminado de enviar:
  //    - Cerrar la conexion si _closeAfterWrite es true.
  //    - O sacar la siguiente respuesta de la cola si existe.

  if (_outBuffer.empty()) {
    if (_closeAfterWrite) {
      _state = STATE_CLOSED;
      return;
    }

    if (!_responseQueue.empty()) {
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
