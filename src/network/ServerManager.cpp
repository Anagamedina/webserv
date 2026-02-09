#include "ServerManager.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>

#include <stdexcept>
#include <unistd.h>

#define CLIENT_TIMEOUT_SECONDS 60

ServerManager::ServerManager(const std::vector< ServerConfig >* configs) :
      configs_(configs) {
    std::set< int > bound_ports;

    if (configs_ == NULL || configs_->empty()) {
        throw std::runtime_error("No servers provided in config list");
    }

    for (size_t i = 0; i < configs_->size(); ++i) {
        const ServerConfig& server = (*configs_)[i];
        int port = server.getPort();

        if (bound_ports.count(port)) {
            continue;
        }
        bound_ports.insert(port);

        TcpListener* listener = new TcpListener(port);
        try {
            listener->listen();
            int fd = listener->getFd();

            listeners_[fd] = listener;
            listener_ports_[fd] = port;

            // El servidor no lee ni escribe datos solo acepta conexiones. (EPOLLIN)
            // Por defecto epoll esta en modo Level Trigger, y para listeners
            // usualmente es lo correcto/seguro.
            epoll_.addFd(fd, EPOLLIN);

            std::cout << "Server listening on port " << port << std::endl;
        } catch (const std::exception& e) {
            delete listener;
            throw;
        }
    }

    if (listeners_.empty()) {
        throw std::runtime_error("No servers could be started (check config ports)");
    }
}

ServerManager::~ServerManager() {

    for (std::map< int, Client* >::iterator it = clients_.begin(); it != clients_.end(); ++it) {
        delete it->second;
    }
    clients_.clear();

    for (std::map< int, TcpListener* >::iterator it = listeners_.begin(); it != listeners_.end();
         ++it) {
        delete it->second;
    }

    std::cout << "ServerManager shut down" << std::endl;
}

void ServerManager::run() {
    epoll_event events[MAX_EVENTS];

    std::cout << "Server started. Waiting for events..." << std::endl;

    while (true) {
        try {
            int num_events = epoll_.wait(events, MAX_EVENTS,
                                         3000); // TODO: 3s timeout for maintenance
                                                // tasks. make it configurable.

            for (int i = 0; i < num_events; ++i) {
                int fd = events[i].data.fd;
                uint32_t event_mask = events[i].events;

                if (listeners_.count(fd)) {
                    handleNewConnection(fd);
                } else if (clients_.count(fd)) {
                    handleClientEvent(fd, event_mask);
                } else if (cgi_pipes_.count(fd)) {
                    handleCgiPipeEvent(fd, event_mask);
                }
            }

            if (num_events == 0) {
                // Idle cycle or timeout, good time to check timeouts
                // TODO: remove debug comment
                std::cout << " Server idle..." << std::endl;
            }
            checkTimeouts();
        } catch (const std::exception& e) {
            std::cerr << "Error in event loop: " << e.what() << std::endl;
        }
    }
}

void ServerManager::checkTimeouts() {
    time_t now = time(NULL);
    std::vector< int > timeout_fds;

    // Iterate over all clients and identify those who timed out
    // TODO: Make timeout configurable via ServerBlock
    double timeout_seconds = CLIENT_TIMEOUT_SECONDS;

    for (std::map< int, Client* >::iterator it = clients_.begin(); it != clients_.end(); ++it) {
        if (difftime(now, it->second->getLastActivity()) > timeout_seconds) {
            timeout_fds.push_back(it->first);
        }
    }

    for (size_t i = 0; i < timeout_fds.size(); ++i) {
        std::cout << "Client " << timeout_fds[i] << " timed out." << std::endl;
        handleClientDisconnect(timeout_fds[i]);
    }
}

void ServerManager::handleNewConnection(int listener_fd) {
    TcpListener* listener = listeners_[listener_fd];
    int port = listener_ports_[listener_fd];

    while (true) {
        int client_fd = listener->acceptConnection();
        if (client_fd == -1)
            break;

        // INFO: Add to Epoll - Level Triggered (no EPOLLET) for safety
        epoll_.addFd(client_fd, EPOLLIN | EPOLLRDHUP);

        Client* new_client = new Client(client_fd, configs_, port);
        new_client->setServerManager(this);
        clients_[client_fd] = new_client;

        std::cout << "New client connected on port " << listener->getPort() << " (FD: " << client_fd
                  << ")" << std::endl;
    }
}

void ServerManager::handleClientEvent(int client_fd, uint32_t events) {
    Client* client = clients_[client_fd];

    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        handleClientDisconnect(client_fd);
        return;
    }

    if (events & EPOLLIN) {
        client->handleRead();
    }

    if (events & EPOLLOUT) {
        client->handleWrite();
    }

    if (client->getState() == STATE_CLOSED) {
        handleClientDisconnect(client_fd);
        return;
    }

    updateClientEvents(client_fd);
}

void ServerManager::updateClientEvents(int client_fd) {
    if (!clients_.count(client_fd))
        return;

    Client* client = clients_[client_fd];
    uint32_t new_events = EPOLLIN | EPOLLRDHUP;
    if (client->needsWrite()) {
        new_events |= EPOLLOUT;
    }
    epoll_.modFd(client_fd, new_events);
}

void ServerManager::handleClientDisconnect(int client_fd) {
    epoll_.removeFd(client_fd);

    if (clients_.count(client_fd)) {
        delete clients_[client_fd];
        clients_.erase(client_fd);
    }

    std::cout << "Client " << client_fd << " disconnected." << std::endl;
}

void ServerManager::handleCgiPipeEvent(int pipe_fd, uint32_t events) {
    if (!cgi_pipes_.count(pipe_fd)) {
        return;
    }

    Client* client = cgi_pipes_[pipe_fd];
    if (client) {
        client->handleCgiPipe(pipe_fd, events);
        updateClientEvents(client->getFd());
    }
}

void ServerManager::registerCgiPipe(int pipe_fd, uint32_t events, Client* client) {
    if (pipe_fd < 0 || client == NULL) {
        return;
    }

    // Add pipe to epoll for monitoring
    epoll_.addFd(pipe_fd, events);

    // Track mapping from pipe FD to Client
    cgi_pipes_[pipe_fd] = client;

    std::cout << "Registered CGI pipe " << pipe_fd << " for events " << events << std::endl;
}

void ServerManager::unregisterCgiPipe(int pipe_fd) {
    if (cgi_pipes_.count(pipe_fd)) {
        epoll_.removeFd(pipe_fd);
        cgi_pipes_.erase(pipe_fd);
        std::cout << "Unregistered CGI pipe " << pipe_fd << std::endl;
    }
}
