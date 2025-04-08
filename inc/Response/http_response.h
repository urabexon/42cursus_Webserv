#ifndef WEBSERV_INCLUDES_HTTP_RESPONSE_H_
#define WEBSERV_INCLUDES_HTTP_RESPONSE_H_

#include <map>
#include <cstring>

#include "../Util/libft.h"

class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  void SetStatus(int code, const std::string& message);
  void SetHeader(const std::string& key, const std::string& value);
  void SetBody(const std::string& body);
  std::string ToString() const;
  void Clear();

  int GetStatusCode() const;
  const std::string& GetStatusMessage() const;
  std::string GetHeader(const std::string& key) const;
  const std::map<std::string, std::string>& GetHeaders() const;
  std::string GetBody() const;

  void SetClientFd(int fd);
  int GetClientFd() const;

  void SetIsCgiResponse(bool is_cgi_response);
  bool GetIsCgiResponse() const;
  void SetIsCgiProcessed(bool is_cgi_processed);
  bool GetIsCgiProcessed() const;

 private:
  int status_code_;
  std::string status_message_;
  std::map<std::string, std::string> headers_;
  std::string body_;
  int clientFd_;
  bool is_cgi_response_;
  bool is_cgi_processed_;
};

#endif
