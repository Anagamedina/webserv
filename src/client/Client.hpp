#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ctime>
#include <string>
#include <vector>

#include "RequestProcessor.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpParser.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

// Representa una conexion TCP con un cliente.
// Se encarga de:
// - Acumular bytes que vienen del socket.
// - Pasarlos al HttpParser.
// - Cuando hay HttpRequest completa, invocar a RequestProcessor.
// - Acumular la respuesta serializada y enviarla por el socket.





// El cliente no puede leer/escribir sin control: necesita saber en que estado
// esta para que el bucle principal escuche eventos.
enum ClientState { 
    STATE_IDLE, // Conexion abierta sin peticion activa (despues de accept).
    STATE_READING_HEADER,
    STATE_READING_BODY,
    STATE_WRITING_RESPONSE,
    STATE_CLOSED
};

class Client {
private:
    Client(const Client&);
    Client& operator=(const Client&);

    int                 _fd;
    std::string         _inBuffer;   // datos recibidos aún sin procesar
    std::string         _outBuffer;  // respuesta lista para enviar (o parcialmente enviada)
    HttpParser          _parser;
    HttpResponse        _response;
    RequestProcessor    _processor;
    const std::vector<ServerConfig>* _configs;
    ClientState         _state;
    time_t              _lastActivity; // para gestionar timeouts

    // Invocado cuando el parser marca una HttpRequest como completa.
    void handleCompleteRequest();

public:
    Client(int fd, const std::vector<ServerConfig>* configs);
    ~Client();

    // Getters
    int getFd() const;
    ClientState getState() const;
    bool needsWrite() const;
    time_t getLastActivity() const;

    // Manejo de eventos
    void handleRead(); 
    void handleWrite();

    // Construccion de respuesta
    void buildResponse();

};

#endif // CLIENT_HPP



