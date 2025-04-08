#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <algorithm>

#include "server_config.h"

class ServerConfig;

class LocationConfig : public BaseConfig {
private:
  std::string path_;
  std::vector<std::string> accepted_methods_;
  std::pair<std::string, int> redirect_;
  std::string script_filename_;
  std::map<std::string, std::string> cgi_executors_;
  int cgi_read_timeout_;
  std::string upload_path_;
  time_t keepalive_timeout_;
  bool keepalive_timeout_set_;

public:
  LocationConfig();
  LocationConfig(const LocationConfig& other);
  LocationConfig(ServerConfig* server_config);
  ~LocationConfig();

  void SetPath(const std::string& path);
  const std::string& GetPath() const;
  void AddAcceptedMethod(const std::string& method);
  void ClearAcceptedMethods();
  const std::vector<std::string>& GetAcceptedMethods() const;
  void SetRedirect(const std::string& url, int code);
  const std::pair<std::string, int>& GetRedirect() const;

  void SetScriptFilename(const std::string& filename);
  const std::string& GetScriptFilename() const;

  void AddCgiExecutor(const std::string& extension, const std::string& executor);
  const std::map<std::string, std::string>& GetCgiExecutors() const;
  std::string GetCgiExecutor(const std::string& extension) const;

  void SetCgiReadTimeout(int timeout);
  int GetCgiReadTimeout() const;
  void SetUploadPath(const std::string& path);
  const std::string& GetUploadPath() const;

  void SetKeepaliveTimeout(time_t timeout);
  time_t GetKeepaliveTimeout() const;
  bool IsKeepaliveTimeoutSet() const;
};

#endif
