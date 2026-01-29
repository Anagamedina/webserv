#include "Client.hpp"
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cerrno>

#define BUFFER_SIZE 4096

Client::Client(int fd, const std::vector<ServerBlock>* configs) 
    : fd_(fd), configs_(configs), active_config_(NULL), state_(CONNECTED), last_activity_(time(NULL)) {
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

    std::cout << "Processing request: " << req.getMethod() << " " << req.getUri() << std::endl;

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
    
    write_buffer_ = response.toString();
    state_ = WRITING;
}

Client::~Client() {
    if (fd_ != -1) {
        close(fd_);
    }
}

int Client::getFd() const {
    return fd_;
}

Client::State Client::getState() const {
    return state_;
}

bool Client::needsWrite() const {
    return state_ == WRITING && !write_buffer_.empty();
}

time_t Client::getLastActivity() const {
    return last_activity_;
}

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
        ssize_t bytes_written = write(fd_, write_buffer_.c_str(), write_buffer_.length());

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
