#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "../http/HttpParser.hpp"
#include "../http/HttpResponse.hpp"

// Representa una conexión TCP con un cliente.
// Se encarga de:
// - Acumular bytes que vienen del socket.
// - Pasarlos al HttpParser.
// - Cuando hay HttpRequest completa, invocar a RequestProcessor.
// - Acumular la respuesta serializada y enviarla por el socket.
class Client {
public:
    // De momento dejamos solo el esqueleto con atributos privados.
private:
    int         _fd;
    std::string _inBuffer;   // datos recibidos aún sin procesar
    std::string _outBuffer;  // respuesta lista para enviar (o parcialmente enviada)
    HttpParser  _parser;

    // Invocado cuando el parser marca una HttpRequest como completa.
    void handleCompleteRequest();
};

#endif // CLIENT_HPP



