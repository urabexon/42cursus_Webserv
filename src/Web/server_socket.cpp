#include "../../inc/Web/server_socket.h"


ServerSocket::ServerSocket() : fd_(kInvalidFd), addr_len_(sizeof(sockaddr_in)) {
  std::memset(&addr_, 0, sizeof(addr_));
}

ServerSocket::ServerSocket(int domain, int type)
    : fd_(kInvalidFd), addr_len_(sizeof(sockaddr_in)) {
  fd_ = socket(domain, type, 0);
  if (fd_ == kInvalidFd) {
    throw std::runtime_error("socket creation failed");
  }

  int opt = 1;
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 )
      {
    throw std::runtime_error("setsockopt failed");
  }
  std::memset(&addr_, 0, sizeof(addr_));
}

ServerSocket::~ServerSocket() { Close(); }

int ServerSocket::getFd() const { return fd_; }

void ServerSocket::setNonBlocking() {
  if (fcntl(fd_, F_SETFL, O_NONBLOCK) < 0) {
    throw std::runtime_error("Failed to set non-blocking");
  }
}

void ServerSocket::Bind(const std::string& host, int port) {
  InitAddr(AF_INET, host, port);
  if (bind(fd_, (struct sockaddr*)&addr_, addr_len_) < 0) {
    std::stringstream ss;
    ss << "bind() to " << host << ":" << port << " failed (" << errno << ": "
       << strerror(errno) << ")";
    throw std::runtime_error(ss.str());
  }
}

void ServerSocket::Listen(int backlog) {
  if (listen(fd_, backlog) < 0) {
    throw std::runtime_error("listen failed");
  }
}

int ServerSocket::Accept() {
  int client_fd = accept(fd_, NULL, NULL);
  if (client_fd < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      throw std::runtime_error("accept failed");
    }
    return kInvalidFd;
  }
  return client_fd;
}

void ServerSocket::Close() {
  if (fd_ != kInvalidFd) {
    close(fd_);
    fd_ = kInvalidFd;
  }
}

void ServerSocket::InitAddr(int domain, const std::string& ip, int port) {
  addr_.sin_family = domain;
  addr_.sin_port = htons(port);

  if (ip.empty() || ip == "*" || ip == "localhost") {
    addr_.sin_addr.s_addr = INADDR_ANY;
    return;
  }

  struct addrinfo hints;
  struct addrinfo* result;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int error = getaddrinfo(ip.c_str(), NULL, &hints, &result);
  if (error != 0) {
    throw std::runtime_error(gai_strerror(error));
  }

  addr_.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
  freeaddrinfo(result);
}
