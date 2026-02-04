#include "Client.hpp"
#include "../network/ServerManager.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

Client::Client(int fd, const std::vector< ServerBlock >* configs)
    : fd_(fd), configs_(configs), active_config_(NULL), state_(CONNECTED),
      cgi_process_(NULL), cgi_response_sent_(0), server_manager_(NULL),
      last_activity_(time(NULL)) {
  if (configs && !configs->empty()) {
    active_config_ = &(*configs)[0]; // Default to first
  }
}

// ... dest, getters ...

void Client::processRequest() {
  const HttpRequest& req = parser_.getRequest();

  // Explicit Chunked Rejection
  if (req.getHeader("transfer-encoding") == "chunked") {
    HttpResponse response(501); // Not Implemented
    response.setBody("Chunked encoding not supported");
    write_buffer_ = response.toString();
    state_ = WRITING;
    return;
  }

  std::cout << "Processing request: " << req.getMethod() << " " << req.getUri()
            << std::endl;

  // Virtual Host Selection
  std::string host = req.getHeader("host");
  if (!host.empty()) {
    // Strip port if present
    size_t colon = host.find(':');
    if (colon != std::string::npos) {
      host = host.substr(0, colon);
    }

    // Find matching server
    for (size_t i = 0; i < configs_->size(); ++i) {
      const ServerBlock& block = (*configs_)[i];
      for (size_t j = 0; j < block.server_names.size(); ++j) {
        if (block.server_names[j] == host) {
          active_config_ = &block;
          break;
        }
      }
    }
  }

  HttpResponse response = processor_.process(req, active_config_);

  // Check if CGI was started
  CgiProcess* cgi_proc = processor_.getCgiProcess();
  if (cgi_proc) {
    // CGI is running - don't send response yet, wait for output
    std::cout << "CGI process started (PID: " << cgi_proc->getPid() << ")"
              << std::endl;
    setCgiProcess(cgi_proc);
    // State changed to WAITING_FOR_CGI in setCgiProcess
    processor_.clearCgiProcess();
    return;
  }

  write_buffer_ = response.toString();
  state_ = WRITING;
}

Client::~Client() {
  if (fd_ != -1) {
    close(fd_);
  }
  if (cgi_process_) {
    // Clean up any remaining pipes
    if (cgi_process_->getPipeIn() != -1) {
      close(cgi_process_->getPipeIn());
    }
    if (cgi_process_->getPipeOut() != -1) {
      close(cgi_process_->getPipeOut());
    }
    delete cgi_process_;
  }
}

int Client::getFd() const { return fd_; }

Client::State Client::getState() const { return state_; }

bool Client::needsWrite() const {
  return state_ == WRITING && !write_buffer_.empty();
}

time_t Client::getLastActivity() const { return last_activity_; }

void Client::handleRead() {
  last_activity_ = time(NULL);
  while (true) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(fd_, buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
      parser_.parse(buffer, bytes_read);
    } else if (bytes_read == 0) {
      state_ = CLOSED;
      return;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Not an error, just no data available right now
        break;
      }
      state_ = CLOSED;
      return;
    }
  }

  if (parser_.getState() == HttpParser::ERROR) {
    HttpResponse response(400);
    response.setBody("Bad Request");
    write_buffer_ = response.toString();
    state_ = WRITING;
    return;
  }

  if (parser_.isComplete()) {
    processRequest();
    // state_ is set to WRITING inside processRequest
  } else {
    state_ = READING;
  }
}

void Client::handleWrite() {
  last_activity_ = time(NULL);

  while (!write_buffer_.empty()) {
    ssize_t bytes_written =
        write(fd_, write_buffer_.c_str(), write_buffer_.length());

    if (bytes_written > 0) {
      write_buffer_.erase(0, bytes_written);
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Socket buffer full, try again later
        return;
      }
      state_ = CLOSED;
      return;
    }
  }

  if (write_buffer_.empty()) {
    // Done writing response

    // Check for Keep-Alive
    const HttpRequest& req = parser_.getRequest();
    std::string connection = req.getHeader("connection");

    // Case-insensitive check (headers are already lowercased by parser)
    bool keep_alive = true;
    if (connection == "close") {
      keep_alive = false;
    } else if (req.getVersion() == "HTTP/1.0") {
      if (connection != "keep-alive") {
        keep_alive = false;
      }
    }

    if (keep_alive) {
      state_ = READING;
      parser_.reset();
    } else {
      state_ = CLOSED;
    }
  }
}

// ============================================================================
// CGI Execution Management
// ============================================================================

void Client::setCgiProcess(CgiProcess* proc) {
  cgi_process_ = proc;
  if (proc && server_manager_) {
    state_ = WAITING_FOR_CGI;
    cgi_response_sent_ = 0;

    // Register output pipe with ServerManager for reading (EPOLLIN)
    server_manager_->registerCgiPipe(proc->getPipeOut(),
                                     EPOLLIN | EPOLLHUP | EPOLLERR, this);

    // Register input pipe if we have body to write
    if (proc->getPipeIn() != -1) {
      if (proc->isRequestBodySent()) {
        // No body to send, close input immediately to signal EOF to CGI
        proc->closePipeIn();
      } else {
        // Register for writing (EPOLLOUT)
        server_manager_->registerCgiPipe(proc->getPipeIn(),
                                         EPOLLOUT | EPOLLHUP | EPOLLERR, this);
      }
    }
  } else if (proc) {
    state_ = WAITING_FOR_CGI;
    cgi_response_sent_ = 0;
  }
}

CgiProcess* Client::getCgiProcess() const { return cgi_process_; }

void Client::setServerManager(ServerManager* manager) {
  server_manager_ = manager;
}

void Client::handleCgiInput() {
  if (!cgi_process_) {
    return;
  }

  int pipe_fd = cgi_process_->getPipeIn();
  if (pipe_fd == -1) {
    return;
  }

  const std::string& body = cgi_process_->getRequestBody();
  size_t written = cgi_process_->getBodyBytesWritten();
  size_t remaining = body.length() - written;

  if (remaining > 0) {
    ssize_t n = write(pipe_fd, body.c_str() + written, remaining);

    if (n > 0) {
      cgi_process_->advanceBodyBytesWritten(n);
      last_activity_ = time(NULL);

      if (cgi_process_->isRequestBodySent()) {
        // Done writing, close pipe to signal EOF
        server_manager_->unregisterCgiPipe(pipe_fd);
        cgi_process_->closePipeIn();
      }
    } else if (n == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Error writing to CGI pipe: " << strerror(errno)
                  << std::endl;
        server_manager_->unregisterCgiPipe(pipe_fd);
        cgi_process_->closePipeIn();
      }
    }
  } else {
    // Should have been closed already if empty
    server_manager_->unregisterCgiPipe(pipe_fd);
    cgi_process_->closePipeIn();
  }
}

void Client::handleCgiPipe(int pipe_fd, uint32_t events) {
  if (!cgi_process_)
    return;

  if (pipe_fd == cgi_process_->getPipeIn()) {
    if (events & EPOLLOUT) {
      handleCgiInput();
    }
    if (events & (EPOLLERR | EPOLLHUP)) {
      server_manager_->unregisterCgiPipe(pipe_fd);
      cgi_process_->closePipeIn();
    }
  } else if (pipe_fd == cgi_process_->getPipeOut()) {
    if (events & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
      handleCgiOutput();
    }
  }
}

void Client::handleCgiOutput() {
  if (!cgi_process_) {
    return;
  }

  // Read available data from CGI pipe (non-blocking)
  int pipe_fd = cgi_process_->getPipeOut();
  if (pipe_fd == -1) {
    return;
  }

  char buffer[4096];
  ssize_t n = read(pipe_fd, buffer, sizeof(buffer));

  if (n > 0) {
    // Data available
    cgi_process_->appendResponseData(buffer, n);

    // Update last activity
    last_activity_ = time(NULL);

  } else if (n == 0) {
    // EOF from CGI pipe - child finished
    std::cout << "CGI process finished" << std::endl;
    finalizeCgiResponse();
    return;

  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    // Error reading
    std::cerr << "Error reading from CGI pipe: " << strerror(errno)
              << std::endl;
    finalizeCgiResponse();
    return;
  }

  // Check for timeout
  if (cgi_process_->isTimedOut()) {
    std::cerr << "CGI process timeout" << std::endl;
    write_buffer_ = "HTTP/1.1 504 Gateway Timeout\r\nContent-Length: 0\r\n\r\n";
    state_ = WRITING;
    if (cgi_process_) {
      int pid = cgi_process_->getPid();
      kill(pid, SIGKILL);
    }
  }
}

void Client::finalizeCgiResponse() {
  if (!cgi_process_) {
    return;
  }

  // Build HTTP response from CGI output
  HttpResponse response(cgi_process_->getStatusCode());

  if (!cgi_process_->isHeadersComplete()) {
    std::cerr << "CGI process failed to produce a valid response headers "
                 "(Premature Exit)"
              << std::endl;
    response = HttpResponse(502);
    response.setBody("Bad Gateway: CGI script did not return a valid response");
    write_buffer_ = response.toString();
    state_ = WRITING;

    // Cleanup follows
    if (cgi_process_->getPipeIn() != -1) {
      server_manager_->unregisterCgiPipe(cgi_process_->getPipeIn());
      cgi_process_->closePipeIn();
    }
    if (cgi_process_->getPipeOut() != -1) {
      server_manager_->unregisterCgiPipe(cgi_process_->getPipeOut());
      cgi_process_->closePipeOut();
    }
    delete cgi_process_;
    cgi_process_ = NULL;

    // Update Epoll to monitor for writing
    if (server_manager_) {
      server_manager_->updateClientEvents(fd_);
    }
    return;
  }

  // Add headers from CGI
  // Parse response headers manually
  const std::string& headers_str = cgi_process_->getResponseHeaders();
  std::istringstream iss(headers_str);
  std::string line;

  while (std::getline(iss, line)) {
    if (!line.empty() && line[line.length() - 1] == '\r') {
      line.erase(line.length() - 1);
    }
    if (line.empty())
      continue;

    size_t colon = line.find(':');
    if (colon != std::string::npos) {
      std::string key = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      // Trim leading spaces
      size_t first = value.find_first_not_of(" \t");
      if (first != std::string::npos) {
        value = value.substr(first);
      }

      // Skip Status header (already handled)
      std::string key_lower = key;
      std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(),
                     ::tolower);
      if (key_lower != "status") {
        response.setHeader(key, value);
      }
    }
  }

  response.setBody(cgi_process_->getResponseBody());

  write_buffer_ = response.toString();
  state_ = WRITING;

  // Cleanup
  if (cgi_process_->getPipeIn() != -1) {
    server_manager_->unregisterCgiPipe(cgi_process_->getPipeIn());
    cgi_process_->closePipeIn();
  }
  if (cgi_process_->getPipeOut() != -1) {
    server_manager_->unregisterCgiPipe(cgi_process_->getPipeOut());
    cgi_process_->closePipeOut();
  }
  delete cgi_process_;
  cgi_process_ = NULL;

  // Update Epoll to monitor for writing
  if (server_manager_) {
    server_manager_->updateClientEvents(fd_);
  }
}
