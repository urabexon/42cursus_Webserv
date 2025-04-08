#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include "../Config/http_config.h"
#include "http_server.h"

class ServerManager {
 public:
  void InitServers(const HttpConfig& config);
  ~ServerManager();

 private:
  typedef std::pair<std::string, int> ServerKey;
  std::map<ServerKey, HttpServer*> server_map_;
  std::vector<HttpServer*> servers_;

  ServerKey MakeServerKey(const std::string& host, int port);

  void CreateServerInstances(const HttpConfig& config);
  void RegisterAndStartServers();
  void HandleInitException(const std::exception& e);
};

#endif
