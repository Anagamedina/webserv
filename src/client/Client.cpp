#include "Client.hpp"

#include <sys/socket.h>
#include <unistd.h>

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
      _responseQueue() {}

Client::~Client() {}

int Client::getFd() const { return _fd; }

ClientState Client::getState() const { return _state; }

bool Client::needsWrite() const { return !_outBuffer.empty(); }

bool Client::hasPendingData() const {
  return !_outBuffer.empty() || !_responseQueue.empty();
}

time_t Client::getLastActivity() const { return _lastActivity; }

// ============================
// LECTURA DESDE EL SOCKET (EPOLLIN)
// ============================
// - Lee bytes crudos con recv().
// - Actualiza _lastActivity.
// - Alimenta el HttpParser.
// - Cuando hay una HttpRequest COMPLETA, llama a handleCompleteRequest().

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

    while (_parser.getState() == COMPLETE) {
      bool shouldClose = handleCompleteRequest();
      if (_cgiProcess) return;
      if (shouldClose) return;
      _parser.reset();
      _parser.consume("");
    }

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

void Client::buildResponse() {
  // Construye la respuesta logica (HttpResponse) a partir de la HttpRequest.
  // Despues, handleCompleteRequest() se encarga de serializarla y encolarla.
  const HttpRequest& request = _parser.getRequest();
  // Primero intentamos el flujo CGI. Si startCgiIfNeeded() devuelve true,
  // significa que ya se ha iniciado un CGI (o se ha respondido con error CGI)
  // y la respuesta final se enviara mas tarde via ClientCgi.cpp.
  if (startCgiIfNeeded(request)) return;
  // Si no es CGI, delegamos en RequestProcessor para servir contenido estatico
  // o errores HTTP clasicos.
  _processor.process(request, _configs, _listenPort,
                     _parser.getState() == ERROR, _response);
}

bool Client::handleCompleteRequest() {
  const HttpRequest& request = _parser.getRequest();
  bool shouldClose =
      (_parser.getState() == ERROR) || request.shouldCloseConnection();
  buildResponse();
  if (_cgiProcess) {
    // Hemos arrancado un CGI: la respuesta HttpResponse se construira
    // cuando termine el CGI (ver finalizeCgiResponse en ClientCgi.cpp).
    // Devolvemos true para indicar al bucle que, por ahora, no debe
    // seguir procesando mas datos de este cliente.
    return true;
  }
  std::vector<char> serialized = _response.serialize();
  enqueueResponse(serialized, shouldClose);
  return shouldClose;
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
