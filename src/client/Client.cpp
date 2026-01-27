#include "Client.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

static std::string httpVersionToString(HttpVersion version)
{
    if (version == HTTP_VERSION_1_0)
        return "HTTP/1.0";
    if (version == HTTP_VERSION_1_1)
        return "HTTP/1.1";
    return "HTTP/1.1";
}

Client::Client(int fd, struct sockaddr_in addr)
    : _fd(fd),
      _inBuffer(),
      _outBuffer(),
      _parser(),
      _response(),
      _state(STATE_IDLE),
      _addr(addr)
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
    while (true)
    {
        bytesRead = recv(_fd, buffer, sizeof(buffer), 0);
        if (bytesRead > 0)
        {
            if (_state == STATE_IDLE)
                _state = STATE_READING_HEADER;
            _parser.consume(std::string(buffer, bytesRead));

            if (_parser.getState() == COMPLETE || _parser.getState() == ERROR)
            {
                handleCompleteRequest();
                break;
            }
        }
        else if (bytesRead == 0)
        {
            _state = STATE_FINISHED;
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            _state = STATE_FINISHED;
            break;
        }
    }
}

//MOTOR DE SALIDA
//ES EL PUNTO CRITICO DE LOS SERVIDORES NO BLOQUEANTES 
//SEND() NO VA ENVAIR Todo EL VECTOR DE UNA VEZ

void Client::handleWrite() {
    while (!_outBuffer.empty())
    {
        ssize_t bytesSent = send(_fd, _outBuffer.c_str(), _outBuffer.size(), 0);
        if (bytesSent > 0)
        {
            _outBuffer.erase(0, bytesSent);
        }
        else if (bytesSent == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            _state = STATE_FINISHED;
            return;
        }
    }

    //1)Intentar enviar lo que queda en el buffer
    //2)hemos terminado de enviar todo?
    //2.1)respuesta completa? cerrar conexion o keep alive
    
    if (_outBuffer.empty())
    {
        const HttpRequest& request = _parser.getRequest();
        bool shouldClose = (_parser.getState() == ERROR) || request.shouldCloseConnection();
        if (shouldClose)
            _state = STATE_FINISHED;
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
    _response = HttpResponse();
    const HttpRequest& request = _parser.getRequest();

    int statusCode = HTTP_STATUS_OK;
    std::string body = "OK\n";

    if (_parser.getState() == ERROR || request.getMethod() == HTTP_METHOD_UNKNOWN)
    {
        statusCode = HTTP_STATUS_BAD_REQUEST;
        body = "Bad Request\n";
    }

    _response.setStatusCode(statusCode);
    _response.setVersion(httpVersionToString(request.getVersion()));
    _response.setHeader("Connection", request.shouldCloseConnection() ? "close" : "keep-alive");
    _response.setHeader("Content-Type", "text/plain");
    _response.setBody(body);
}

void Client::handleCompleteRequest()
{
    buildResponse();
    std::vector<char> serialized = _response.serialize();
    _outBuffer.assign(serialized.begin(), serialized.end());
    _state = STATE_WRITING_RESPONSE;
}

