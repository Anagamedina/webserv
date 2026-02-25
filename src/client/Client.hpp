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

enum ClientState {
  STATE_IDLE,            // Sin petición activa
  STATE_READING_HEADER,  // Leyendo headers del cliente
  STATE_READING_BODY,    // Leyendo body (POST, etc.)
  STATE_WRITING_RESPONSE,
  STATE_CLOSED
};

struct PendingResponse {
  std::string data;
  bool closeAfter;
  PendingResponse(const std::string& d, bool c) : data(d), closeAfter(c) {}
};

// -----------------------------------------------------------------------------
// CLIENT - Representa una conexión TCP con un cliente
// -----------------------------------------------------------------------------
class Client {
  bool _savedShouldClose;
  HttpVersion _savedVersion;

 public:
  // ---- Constructor y destructor ----
  Client(int fd, const std::vector<ServerConfig>* configs, int listenPort, const std::string& clientIp = "127.0.0.1");
  ~Client();

  // ---- Getters (para que el bucle principal sepa el estado) ----
  int getFd() const;
  ClientState getState() const;
  bool needsWrite() const;
  bool hasPendingData() const;
  time_t getLastActivity() const;
  bool isRequestInProgress() const;
  double getRequestElapsedSeconds() const;

  // ---- Manejo de eventos (llamados desde ServerManager/epoll) ----
  void setServerManager(ServerManager* serverManager);
  void handleRead();
  void handleWrite();
  void handleCgiPipe(int pipe_fd, size_t events);
  bool checkCgiTimeout();
  bool checkRequestTimeout();

  // ---- Construcción de respuesta (llamado internamente) ----
  void buildResponse();

 private:
  Client(const Client&);
  Client& operator=(const Client&);

  // ---- Datos del socket y conexión ----
  int _fd;
  int _listenPort;
  std::string _clientIp;
  const std::vector<ServerConfig>* _configs;
  ClientState _state;
  time_t _lastActivity;
  time_t _requestStartTime;
  bool _forceCloseCurrentResponse;

  // ---- Buffers ----
  std::string _outBuffer;  // Respuesta lista para enviar
  std::queue<PendingResponse> _responseQueue;

  // ---- Parser y respuesta HTTP ----
  HttpParser _parser;
  HttpResponse _response;
  RequestProcessor _processor;

  // ---- CGI (si hay script en ejecución) ----
  ServerManager* _serverManager;
  CgiProcess* _cgiProcess;
  const ServerConfig* _cgiServerConfig;

  // ---- Flags ----
  bool _closeAfterWrite;
  bool _sent100Continue;  // Para Expect: 100-continue

  // ---- Funciones auxiliares (solo usadas dentro de la clase) ----
  bool
  handleCompleteRequest();  // Request parseada → construir y encolar respuesta
  void enqueueResponse(const std::vector<char>& data, bool closeAfter);
  void handleExpect100();  // Expect: 100-continue
  bool startCgiIfNeeded(const HttpRequest& request);
  void finalizeCgiResponse();

  // Invocado cuando el parser marca una HttpRequest como completa.
  void finalizeCgiResponse(const CgiProcess* finishedProcess);
  void processRequests();
  //
  // Invocado cuando el parser marca una HttpRequest como completa.
  void dispatchAction(const HttpRequest& request,
                      const RequestProcessor::ProcessingResult& result);
  bool startCgi(const RequestProcessor::CgiInfo& cgiInfo);

  bool executeCgi(const RequestProcessor::CgiInfo& cgiInfo);
};

#endif  // CLIENT_HPP
