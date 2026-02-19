#pragma once

#include <string>

class TcpListener {
 public:
  TcpListener(const std::string& host, int port);
  ~TcpListener();

  void listen();

  int acceptConnection();

  int getFd() const;

  int getPort() const;

 private:
  int socket_fd_;
  int port_;
  std::string host_;

  void createSocket();
  void setSocketOptions();
  void bindSocket();

  // Disable copying
  TcpListener(const TcpListener&);
  TcpListener& operator=(const TcpListener&);
};
