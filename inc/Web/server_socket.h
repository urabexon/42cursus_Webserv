#ifndef SERVER_SOCKET_HPP
#define SERVER_SOCKET_HPP

#include <fcntl.h>
#include <netdb.h>
#include <stdexcept>
#include <sstream>
#include <cerrno>
#include <cstring>
#include <unistd.h>

class ServerSocket {
 public:
  static const int kInvalidFd = -1;

  ServerSocket();
  ServerSocket(int domain, int type);
  ~ServerSocket();

  int getFd() const;
  void setNonBlocking();

  void Bind(const std::string& host, int port);
  void Listen(int backlog);
  int Accept();
  void Close();

 private:
  void InitAddr(int domain, const std::string& ip, int port);

 private:
  int fd_;
  struct sockaddr_in addr_;
  socklen_t addr_len_;
};

#endif
