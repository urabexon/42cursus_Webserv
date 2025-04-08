#ifndef HTTP_CONFIG_HPP
#define HTTP_CONFIG_HPP

#include "server_config.h"

struct ListenDirective;
class ServerConfig;

class HttpConfig : public BaseConfig {
private:
  std::vector<ServerConfig*> servers_;
  time_t keepalive_timeout_;
  bool keepalive_timeout_set_;

  bool HasServerNameConflict(const ServerConfig* server) const;
  bool HasConflictBetweenServers(const ServerConfig* new_server, const ServerConfig* existing_server) const;
  bool AreListenDirectivesOverlapping(const ListenDirective& first, const ListenDirective& second) const;
  bool DoServerNamesConflict(const ServerConfig* first_server, const ServerConfig* second_server) const;
  void PrintServerNameConflictWarning(const ServerConfig* server) const;

public:
  HttpConfig();
  HttpConfig(const HttpConfig& other);
  ~HttpConfig();

  HttpConfig& operator=(const HttpConfig& other);
  const std::vector<ServerConfig*>& GetServers() const;
  void AddServer(ServerConfig* server);

  time_t GetKeepaliveTimeout() const;
  void SetKeepaliveTimeout(time_t timeout);
  bool IsKeepaliveTimeoutSet() const;
};

#endif
