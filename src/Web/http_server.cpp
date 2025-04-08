#include "../../inc/Web/http_server.h"

HttpServer::HttpServer(const std::string& host, int port)
    : listen_socket_(AF_INET, SOCK_STREAM) {
  listen_socket_.Bind(host, port);
  listen_socket_.Listen(kMaxEvents);
  listen_socket_.setNonBlocking();
}

HttpServer::~HttpServer() {
  for (std::map<int, ClientConnection*>::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    delete it->second;
  }
  connections_.clear();
}

void HttpServer::OnEvent(uint32_t events) {
  if (events & EPOLLIN) {
    AcceptNewClient();
  }
}

int HttpServer::getFd() const {
  return listen_socket_.getFd();
}

void HttpServer::AddServerConfig(ServerConfig* config) {
  server_config_ = config;
}

void HttpServer::Start() {
  EpollHandler::Instance().RegisterEvent(this, EPOLLIN);
}

std::map<int, ClientConnection*>& HttpServer::GetConnections() {
  return connections_;
}

void HttpServer::AcceptNewClient() {
  int client_fd = listen_socket_.Accept();
  if (client_fd < 0) return;

  std::map<int, ClientConnection*>::iterator it = connections_.find(client_fd);
  if (it != connections_.end()) {
    if (it->second) {
      if (!it->second->IsClosed()) {
        EpollHandler::Instance().UnregisterEvent(it->second);
        it->second->Close();
      }

      EpollHandler::Instance().ScheduleForDeletion(it->second);
    }

    connections_.erase(it);
  }

  ClientConnection* client = new ClientConnection(client_fd, server_config_);
  connections_[client_fd] = client;
  EpollHandler::Instance().RegisterEvent(client, EPOLLIN);
}
