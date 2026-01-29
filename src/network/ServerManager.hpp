#pragma once

#include "../client/Client.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpParser.hpp"
#include "EpollWrapper.hpp"
#include "TcpListener.hpp"
#include <map>
#include <vector>

class ServerManager {
public:
  ServerManager(const ServerConfig &config);
  ~ServerManager();

  void run();

private:
  // Maximum number of events to process at once
  static const int MAX_EVENTS = 64;

  // Disable copying
  ServerManager(const ServerManager &);
  ServerManager &operator=(const ServerManager &);

  // Event handlers
  void handleNewConnection(int listener_fd);
  void handleClientEvent(int client_fd, uint32_t events);
  void handleClientDisconnect(int client_fd);
  void checkTimeouts();

  EpollWrapper epoll_;

  std::map<int, TcpListener *> listeners_;
  // Map listener FD to port, or directly to configs?
  // Map Port -> Vector of ServerBlocks
  std::map<int, std::vector<ServerBlock>> port_configs_;
  // Map Listener FD -> Port (to look up configs)
  std::map<int, int> listener_ports_;

  // Active clients
  std::map<int, Client *> clients_;
};
