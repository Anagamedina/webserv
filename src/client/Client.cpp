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
      _lastActivity(std::time(0))
{
}

Client::~Client()
{
}

int Client::getFd() const
{
    return _fd;
}

ClientState Client::getState() const
{
    return _state;
}

bool Client::needsWrite() const
{
    return !_outBuffer.empty();
}

time_t Client::getLastActivity() const
{
    return _lastActivity;
}


//MOTOR DE ENTRADA  
//ESTE METODO SE LLAMARA CUANDO EPOLL NOS AVISE CUANDO HAY EPOLLIN

void Client::handleRead()
{
    char buffer[4096]; //buffer temporal
    ssize_t bytesRead = 0;

    //1) Recibir datos crudos
    //2)actualizar tiempo para evitar timeout 
    //3) anadir al buffer de procesamiento 
    //4) invocar al parser (la logia que ya esta hecha)
    //5)verificar si el parser termino 
    bytesRead = recv(_fd, buffer, sizeof(buffer), 0);
    if (bytesRead > 0)
    {
    	//TODO crear una macro maxtime
        _lastActivity = std::time(0);
        if (_state == STATE_IDLE)
            _state = STATE_READING_HEADER;
        _parser.consume(std::string(buffer, bytesRead));

        if (_parser.getState() == COMPLETE || _parser.getState() == ERROR)
            handleCompleteRequest();
    }
    else if (bytesRead == 0)
    {
        _state = STATE_CLOSED;
    }
    else
    {
        _state = STATE_CLOSED;
    }
}

//MOTOR DE SALIDA
//ES EL PUNTO CRITICO DE LOS SERVIDORES NO BLOQUEANTES 
//SEND() NO VA ENVAIR Todo EL VECTOR DE UNA VEZ

void Client::handleWrite() {
    if (_outBuffer.empty())
        return;

    ssize_t bytesSent = send(_fd, _outBuffer.c_str(), _outBuffer.size(), 0);
    if (bytesSent > 0)
    {
        _lastActivity = std::time(0);
        _outBuffer.erase(0, bytesSent);
    }
    else if (bytesSent < 0)
    {
        _state = STATE_CLOSED;
        return;
    }

    //1)Intentar enviar lo que queda en el buffer
    //2)hemos terminado de enviar todo?
    //2.1)respuesta completa? cerrar conexion o keep alive
    
    if (_outBuffer.empty())
    {
        const HttpRequest& request = _parser.getRequest();
        bool shouldClose = (_parser.getState() == ERROR) || request.shouldCloseConnection();
        if (shouldClose)
            _state = STATE_CLOSED;
        else
        {
            _parser.reset();
            _inBuffer.clear();
            _state = STATE_IDLE;
        }
    }
}


void Client::buildResponse() {
    //depende del status que tengas respondemos una cosa u otra...
    //serializar a raw bytes para el envio 
    const HttpRequest& request = _parser.getRequest();
    _processor.process(request, _configs, _listenPort, _parser.getState() == ERROR, _response);
}

void Client::handleCompleteRequest()
{
    buildResponse();
    std::vector<char> serialized = _response.serialize();
    _outBuffer.assign(serialized.begin(), serialized.end());
    _state = STATE_WRITING_RESPONSE;
}

