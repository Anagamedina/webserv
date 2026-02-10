#pragma once

#include <map>
#include <vector>

#include "../client/Client.hpp"
#include "../config/ServerConfig.hpp"
#include "EpollWrapper.hpp"
#include "TcpListener.hpp"

class ServerManager {
 public:
  ServerManager(const std::vector<ServerConfig>* configs);
  ~ServerManager();

  void run();

  // CGI pipe registration (called by Client when starting CGI)
  void updateClientEvents(int client_fd);

  void registerCgiPipe(int pipe_fd, uint32_t events, Client* client);
  void unregisterCgiPipe(int pipe_fd);

 private:
  // Maximum number of events to process at once
  static const int MAX_EVENTS = 64;

  // Disable copying
  ServerManager(const ServerManager&);
  ServerManager& operator=(const ServerManager&);

  // Event handlers
  void handleNewConnection(int listener_fd);
  void handleClientEvent(int client_fd, uint32_t events);
  void handleClientDisconnect(int client_fd);
  void handleCgiPipeEvent(int pipe_fd,
                          uint32_t events);  // NEW: Handle CGI output
  void checkTimeouts();

  EpollWrapper epoll_;

  std::map<int, TcpListener*> listeners_;
  // Map listener FD to port, or directly to configs?
  // Config list provided by ConfigParser
  const std::vector<ServerConfig>* configs_;
  // Map Listener FD -> Port
  std::map<int, int> listener_ports_;

  // Active clients
  std::map<int, Client*> clients_;

  // Map CGI pipe FD -> Client (for CGI output handling)
  std::map<int, Client*> cgi_pipes_;  // NEW
                                      //
};
