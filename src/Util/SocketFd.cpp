#include "Util/SocketFd.h"

SocketFd::SocketFd(int fd) : fd_(fd) {}

SocketFd::~SocketFd() {
  closeIfValid();
}

int SocketFd::get() const {
  return fd_;
}

void SocketFd::reset(int new_fd) {
  closeIfValid();
  fd_ = new_fd;
}

int SocketFd::release() {
  int temp = fd_;
  fd_ = -1;
  return temp;
}

void SocketFd::closeIfValid() {
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

bool SocketFd::valid() const {
  return fd_ >= 0;
}
