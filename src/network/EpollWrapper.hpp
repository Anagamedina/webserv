
#pragma once

#include <sys/epoll.h>
#include <unistd.h>

#include <stdexcept>
#include <string>
#include <vector>

class EpollWrapper {
 public:
  EpollWrapper();
  ~EpollWrapper();

  void addFd(int fd, uint32_t events);
  void modFd(int fd, uint32_t events);

  void removeFd(int fd);

  int wait(epoll_event* events, int maxevents, int timeout);

  int getFd() const;

 private:
  int epoll_fd_;

  // Disable copying
  EpollWrapper(const EpollWrapper&);
  EpollWrapper& operator=(const EpollWrapper&);
};
