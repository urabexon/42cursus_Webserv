#ifndef SOCKET_FD_H
#define SOCKET_FD_H

#include <unistd.h>

class SocketFd {
 private:
  int fd_;
 public:
  explicit SocketFd(int fd = -1);
  ~SocketFd();

  int get() const;
  void reset(int new_fd);
  int release();
  void closeIfValid();
  bool valid() const;

 private:
  SocketFd(const SocketFd&);
  SocketFd& operator=(const SocketFd&);
};

#endif // SOCKET_FD_H
