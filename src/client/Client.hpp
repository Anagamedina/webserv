#pragma once

#include "../cgi/CgiProcess.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpParser.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "RequestProcessor.hpp"
#include <ctime>
#include <string>

// Forward declaration
class ServerManager;

class Client {
public:
  enum State {
    CONNECTED,
    READING,
    WRITING,
    WAITING_FOR_CGI, // NEW: Waiting for CGI output
    KEEP_ALIVE,
    CLOSED
  };

  Client(int fd, const std::vector< ServerBlock >* configs);
  ~Client();

  // Core event handlers
  void handleRead();
  void handleWrite();
  void handleCgiOutput(); // NEW: Handle CGI pipe events

  // CGI management
  void setCgiProcess(CgiProcess* proc);
  CgiProcess* getCgiProcess() const;
  void setServerManager(ServerManager* manager); // NEW: Set server manager for
                                                 // CGI pipe registration
  void finalizeCgiResponse();                    // NEW: Send final CGI response

  // Getters
  int getFd() const;
  State getState() const;
  bool needsWrite() const;
  time_t getLastActivity() const;

  // Buffer management
  void appendToReadBuffer(const char* data, size_t size);

private:
  int fd_;
  const std::vector< ServerBlock >* configs_;
  const ServerBlock* active_config_;
  State state_;

  // Buffers
  std::string read_buffer_;
  std::string write_buffer_;

  // Components
  HttpParser parser_;
  RequestProcessor processor_;

  // CGI execution (NEW)
  CgiProcess* cgi_process_;
  size_t cgi_response_sent_;      // Track how much CGI response we've written
  ServerManager* server_manager_; // NEW: Reference to server manager for CGI
                                  // pipe registration

  // Timeouts
  time_t last_activity_;

  // Helpers
  void processRequest();
};
