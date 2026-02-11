#include "EpollWrapper.hpp"

#include <cerrno>
#include <iostream>

int EpollWrapper::getFd() const { return epoll_fd_; }

EpollWrapper::EpollWrapper() {
  // epoll_create(size) size is ignored in modern kernels but must be > 0.
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    throw std::runtime_error("Failed to create epoll instance");
  }
  std::cout << "Epoll created successfully (fd: " << epoll_fd_ << ")"
            << std::endl;
}

EpollWrapper::~EpollWrapper() {
  if (epoll_fd_ != -1) {
    close(epoll_fd_);
  }
}

void EpollWrapper::addFd(int fd, uint32_t events) {
  epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
    throw std::runtime_error("Failed to add fd to epoll");
  }
}

void EpollWrapper::modFd(int fd, uint32_t events) {
  epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
    throw std::runtime_error("Failed to modify fd in epoll");
  }
}

void EpollWrapper::removeFd(int fd) {
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
    std::cerr << "Warning: Failed to remove fd from epoll" << std::endl;
  }
}

int EpollWrapper::wait(epoll_event* events, int maxevents, int timeout) {
  int num_events = epoll_wait(epoll_fd_, events, maxevents, timeout);
  if (num_events == -1) {
    // Don't throw on EINTR (interrupted by signal)
    if (errno != EINTR) {
      throw std::runtime_error("epoll_wait failed");
    }
    return 0;
  }
  return num_events;
}
