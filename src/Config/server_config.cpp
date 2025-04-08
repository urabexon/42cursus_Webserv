#include "../../inc/Config/server_config.h"

ServerConfig::ServerConfig() : is_default_(false), keepalive_timeout_(-1), keepalive_timeout_set_(false), http_config_(NULL) {
}

ServerConfig::ServerConfig(const ServerConfig& other) : BaseConfig(other),
  locations_(other.locations_),
  server_names_(other.server_names_),
  is_default_(other.is_default_),
  keepalive_timeout_(other.keepalive_timeout_),
  keepalive_timeout_set_(other.keepalive_timeout_set_),
  redirect_(other.redirect_),
  listen_directives_(other.listen_directives_),
  http_config_(other.http_config_) {
}

ServerConfig::ServerConfig(HttpConfig* http_config) : BaseConfig(*http_config),
  is_default_(false),
  keepalive_timeout_(http_config->GetKeepaliveTimeout()),
  keepalive_timeout_set_(false),
  http_config_(http_config) {
    autoindex_set_ = false;
}

ServerConfig::~ServerConfig() {
  for (std::map<std::string, LocationConfig*>::iterator it = locations_.begin();
       it != locations_.end(); ++it) {
    delete it->second;
  }
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
  if (this != &other) {
    BaseConfig::operator=(other);
    listen_directives_ = other.listen_directives_;
    server_names_ = other.server_names_;
    is_default_ = other.is_default_;
    keepalive_timeout_ = other.keepalive_timeout_;
    keepalive_timeout_set_ = other.keepalive_timeout_set_;
    redirect_ = other.redirect_;
    http_config_ = other.http_config_;
  }
  return *this;
}

const HttpConfig* ServerConfig::GetHttpConfig() const {
  return http_config_;
}

void ServerConfig::AddServerName(const std::string& name) {
  server_names_.push_back(name);
}

const std::vector<std::string>& ServerConfig::GetServerNames() const {
  return server_names_;
}

void ServerConfig::SetDefault(bool is_default) {
  is_default_ = is_default;
}

bool ServerConfig::IsDefault() const {
  return is_default_;
}

void ServerConfig::SetKeepaliveTimeout(time_t timeout) {
  keepalive_timeout_ = timeout;
  keepalive_timeout_set_ = true;
}

time_t ServerConfig::GetKeepaliveTimeout() const {
  return keepalive_timeout_;
}

bool ServerConfig::IsKeepaliveTimeoutSet() const {
  return keepalive_timeout_set_;
}

void ServerConfig::SetRedirect(const std::string& url, int code) {
  redirect_ = std::make_pair(url, code);
}

const std::pair<std::string, int>& ServerConfig::GetRedirect() const {
  return redirect_;
}

void ServerConfig::AddListenDirective(const std::string& host, int port) {
  listen_directives_.push_back(ListenDirective(host, port));
}

const std::vector<ListenDirective>& ServerConfig::GetListenDirectives() const {
  return listen_directives_;
}

const std::map<std::string, LocationConfig*>& ServerConfig::GetLocations() const {
  return locations_;
}

void ServerConfig::AddLocation(LocationConfig* location) {
  std::map<std::string, LocationConfig*>::iterator it = locations_.find(location->GetPath());
  if (it != locations_.end()) {
    std::string error_msg = "duplicate location \"" + location->GetPath() + "\"";
    throw std::runtime_error(error_msg);
  }
  locations_[location->GetPath()] = location;
}
