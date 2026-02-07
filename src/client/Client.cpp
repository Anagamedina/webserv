#include "Client.hpp"

#include <sys/socket.h>
#include <unistd.h>

Client::Client(int fd, const std::vector< ServerConfig >* configs, int listenPort) :
      _fd(fd),
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
      _responseQueue() {
}

Client::~Client() {
    /*
    if (_cgiProcess)
    {
        delete _cgiProcess;
        _cgiProcess = 0;
    }
  */
}

int Client::getFd() const {
    return _fd;
}

ClientState Client::getState() const {
    return _state;
}

bool Client::needsWrite() const {
    return !_outBuffer.empty();
}

time_t Client::getLastActivity() const {
    return _lastActivity;
}

// MOTOR DE ENTRADA
// ESTE METODO SE LLAMARA CUANDO EPOLL NOS AVISE CUANDO HAY EPOLLIN

void Client::handleRead() {
    char buffer[4096]; // buffer temporal
    ssize_t bytesRead = 0;

    // 1) Recibir datos crudos
    // 2)actualizar tiempo para evitar timeout
    // 3) anadir al buffer de procesamiento
    // 4) invocar al parser (la logia que ya esta hecha)
    // 5)verificar si el parser termino
    bytesRead = recv(_fd, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        _lastActivity = std::time(0);
        if (_state == STATE_IDLE)
            _state = STATE_READING_HEADER;

        _parser.consume(std::string(buffer, bytesRead));

        while (_parser.getState() == COMPLETE) {
            bool shouldClose = handleCompleteRequest();
            if (_cgiProcess)
                return;
            if (shouldClose)
                return;
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

// MOTOR DE SALIDA
// ES EL PUNTO CRITICO DE LOS SERVIDORES NO BLOQUEANTES
// SEND() NO VA ENVAIR Todo EL VECTOR DE UNA VEZ

void Client::handleWrite() {
    if (_outBuffer.empty())
        return;

    ssize_t bytesSent = send(_fd, _outBuffer.c_str(), _outBuffer.size(), 0);
    if (bytesSent > 0) {
        _lastActivity = std::time(0);
        _outBuffer.erase(0, bytesSent);
    } else if (bytesSent < 0) {
        _state = STATE_CLOSED;
        return;
    }

    // 1)Intentar enviar lo que queda en el buffer
    // 2)hemos terminado de enviar todo?
    // 2.1)respuesta completa? cerrar conexion o keep alive

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
    // depende del status que tengas respondemos una cosa u otra...
    // serializar a raw bytes para el envio
    const HttpRequest& request = _parser.getRequest();
    // TODO (CGI flujo):
    // 1) RequestProcessor decide si es CGI o estatico.
    // 2) Si es CGI, el Client debe crear el proceso CGI (CgiExecutor)
    //    y registrar el pipe de salida en epoll via ServerManager.
    // 3) ServerManager recibe eventos del pipe y llama de vuelta al Client
    //    para leer la salida CGI y construir HttpResponse.
    //
    // Ejemplo:
    // if (requestIsCgi(request)) {
    //     CgiExecutor exec;
    //     CgiProcess* proc = exec.executeAsync(request, scriptPath, interpreterPath);
    //     if (proc) {
    //         serverManager->registerCgiPipe(proc->getPipeOut(), EPOLLIN, this);
    //         // Guardar proc en el Client para leer salida luego
    //         // this->_cgiProcess = proc;
    //     } else {
    //         buildErrorResponse(_response, request, 500, true, /*server*/0);
    //     }
    //     return;
    // }
    // else {
    //     _processor.process(request, _configs, _listenPort,
    //                        _parser.getState() == ERROR, _response);
    // }
    if (startCgiIfNeeded(request))
        return;
    _processor.process(request, _configs, _listenPort, _parser.getState() == ERROR, _response);
}

bool Client::handleCompleteRequest() {
    const HttpRequest& request = _parser.getRequest();
    bool shouldClose = (_parser.getState() == ERROR) || request.shouldCloseConnection();
    buildResponse();
    if (_cgiProcess) {
        // Esperando respuesta CGI; la respuesta se enviara cuando termine.
        // Ejemplo (comentado):
        // if (_cgiProcess->isHeadersComplete()) {
        //     // Ya tenemos headers+body del CGI en buffers internos.
        //     finalizeCgiResponse();
        //     _cgiProcess = 0;
        // }
        return true;
    }
    std::vector< char > serialized = _response.serialize();
    enqueueResponse(serialized, shouldClose);
    return shouldClose;
}

void Client::enqueueResponse(const std::vector< char >& data, bool closeAfter) {
    std::string payload(data.begin(), data.end());
    if (_outBuffer.empty()) {
        _outBuffer = payload;
        _closeAfterWrite = closeAfter;
        _state = STATE_WRITING_RESPONSE;
        return;
    }
    _responseQueue.push(PendingResponse(payload, closeAfter));
}
