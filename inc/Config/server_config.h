#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <cstring>
#include <stdexcept>

#include "base_config.h"
#include "location_config.h"
#include "http_config.h"

class HttpConfig;
class LocationConfig;

struct ListenDirective {
  std::string host;
  int port;
  ListenDirective(const std::string& h, int p) : host(h), port(p) {}
};

class ServerConfig : public BaseConfig {
 private:
  std::map<std::string, LocationConfig*> locations_;
  std::vector<std::string> server_names_;
  bool is_default_;
  time_t keepalive_timeout_;
  bool keepalive_timeout_set_;
  std::pair<std::string, int> redirect_;
  std::vector<ListenDirective> listen_directives_;
  HttpConfig* http_config_;

 public:
  ServerConfig();
  ServerConfig(const ServerConfig& other);
  ~ServerConfig();
  ServerConfig& operator=(const ServerConfig& other);
  ServerConfig(HttpConfig* http_config);

  void AddServerName(const std::string& name);
  const std::vector<std::string>& GetServerNames() const;
  void SetDefault(bool is_default);
  bool IsDefault() const;
  void SetKeepaliveTimeout(time_t timeout);
  time_t GetKeepaliveTimeout() const;
  bool IsKeepaliveTimeoutSet() const;
  void SetRedirect(const std::string& url, int code);
  const std::pair<std::string, int>& GetRedirect() const;
  void AddListenDirective(const std::string& host, int port);
  const std::vector<ListenDirective>& GetListenDirectives() const;
  const std::map<std::string, LocationConfig*>& GetLocations() const;
  void AddLocation(LocationConfig* location);
  const HttpConfig* GetHttpConfig() const;
};

#endif
