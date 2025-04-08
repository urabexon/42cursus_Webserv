#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "client_connection.h"
#include "server_socket.h"
#include "event.h"
#include "../Request/request_parser.h"

class ClientConnection;

class HttpServer : public Event {
 private:
  ServerSocket listen_socket_;
  std::map<int, ClientConnection*> connections_;
  ServerConfig* server_config_;
  static const int kMaxEvents = 1024;

 public:
  HttpServer(const std::string& host, int port);
  ~HttpServer();

  void OnEvent(uint32_t events);
  int getFd() const;

  void AddServerConfig(ServerConfig* config);
  void Start();
  std::map<int, ClientConnection*>& GetConnections();

 private:
  void AcceptNewClient();
};

#endif
