#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ctime>
#include <queue>
#include <string>
#include <vector>

#include "RequestProcessor.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpParser.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

class ServerManager;
class CgiProcess;

// Representa una conexion TCP con un cliente.
// Se encarga de:
// - Acumular bytes que vienen del socket.
// - Pasarlos al HttpParser.
// - Cuando hay HttpRequest completa, invocar a RequestProcessor.
// - Acumular la respuesta serializada y enviarla por el socket.

// El cliente no puede leer/escribir sin control: necesita saber en que estado
// esta para que el bucle principal escuche eventos.
enum ClientState {
  STATE_IDLE,  // Conexion abierta sin peticion activa (despues de accept).
  STATE_READING_HEADER,
  STATE_READING_BODY,
  STATE_WRITING_RESPONSE,
  STATE_CLOSED
};

class Client {
 private:
  Client(const Client&);
  Client& operator=(const Client&);

  int _fd;
  std::string _inBuffer;  // datos recibidos aún sin procesar
  std::string
      _outBuffer;  // respuesta lista para enviar (o parcialmente enviada)
  HttpParser _parser;
  HttpResponse _response;
  RequestProcessor _processor;
  const std::vector<ServerConfig>* _configs;
  int _listenPort;
  ClientState _state;
  time_t _lastActivity;  // para gestionar timeouts
  ServerManager* _serverManager;
  CgiProcess* _cgiProcess;
  bool _closeAfterWrite;
  bool _sent100Continue;  // Para Expect: 100-continue (respuesta intermedia)
  struct PendingResponse {
    std::string data;
    bool closeAfter;
    PendingResponse(const std::string& d, bool c) : data(d), closeAfter(c) {}
  };
  std::queue<PendingResponse> _responseQueue;

  // Invocado cuando el parser marca una HttpRequest como completa.
  bool handleCompleteRequest();
  void enqueueResponse(const std::vector<char>& data, bool closeAfter);
  void handleExpect100();
  bool startCgiIfNeeded(const HttpRequest& request);
  void finalizeCgiResponse();

 public:
  Client(int fd, const std::vector<ServerConfig>* configs, int listenPort);
  ~Client();

  // Getters
  int getFd() const;
  ClientState getState() const;
  bool needsWrite() const;
  bool hasPendingData() const;
  time_t getLastActivity() const;

  // Manejo de eventos
  void handleRead();
  void handleWrite();
  void handleCgiPipe(int pipe_fd, size_t events);
  void setServerManager(ServerManager* serverManager);

  // Construccion de respuesta
  void buildResponse();
};

#endif  // CLIENT_HPP
