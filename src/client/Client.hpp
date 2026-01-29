#pragma once

#include "../config/ServerConfig.hpp"
#include "../http/HttpParser.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "RequestProcessor.hpp"
#include <string>
#include <ctime>

class Client {
public:
    enum State {
        CONNECTED,
        READING,
        WRITING,
        KEEP_ALIVE,
        CLOSED
    };

    Client(int fd, const std::vector<ServerBlock>* configs);
    ~Client();

    // Core event handlers
    void handleRead();
    void handleWrite();

    // Getters
    int getFd() const;
    State getState() const;
    bool needsWrite() const;
    time_t getLastActivity() const;

    // Buffer management
    void appendToReadBuffer(const char* data, size_t size);
    
private:
    int fd_;
    const std::vector<ServerBlock>* configs_;
    const ServerBlock* active_config_;
    State state_;
    
    // Buffers
    std::string read_buffer_;
    std::string write_buffer_;
    
    // Components
    HttpParser parser_;
    RequestProcessor processor_;
    
    // Timeouts
    time_t last_activity_;

    // Helpers
    void processRequest();

};
