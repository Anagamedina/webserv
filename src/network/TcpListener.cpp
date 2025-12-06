#include "TcpListener.hpp"
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils/StringUtils.hpp"

TcpListener::TcpListener(int port) : socket_fd_(-1), port_(port) {
    createSocket();
    setSocketOptions();
    bindSocket();
}

TcpListener::~TcpListener() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
    }
}

void TcpListener::createSocket() {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    std::cout << "Socket created (fd: " << socket_fd_ << ")" << std::endl;
}

void TcpListener::setSocketOptions() {
    int opt = 1;
    // Allow socket descriptor to be reusable
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(socket_fd_);
        throw std::runtime_error("Failed to set socket options");
    }

    // Set socket to non-blocking mode
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1) {
        close(socket_fd_);
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(socket_fd_);
        throw std::runtime_error("Failed to set socket to non-blocking");
    }
}

void TcpListener::bindSocket() {
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    address.sin_port = htons(port_);

    if (bind(socket_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(socket_fd_);
        throw std::runtime_error("Failed to bind socket to port " + string_utils::toString(port_));
    }
    
    std::cout << "Socket bound to port " << port_ << std::endl;
}

void TcpListener::listen() {
    if (::listen(socket_fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
    std::cout << "Listening for connections..." << std::endl;
}

int TcpListener::acceptConnection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(socket_fd_, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
        }
        return -1;
    }

    // Set client socket to non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port)
              << " (fd: " << client_fd << ")" << std::endl;

    return client_fd;
}

int TcpListener::getFd() const {
    return socket_fd_;
}

int TcpListener::getPort() const {
    return port_;
}
