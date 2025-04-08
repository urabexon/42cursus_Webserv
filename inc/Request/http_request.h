#ifndef WEBSERV_inc_HTTP_REQUEST_H_
#define WEBSERV_inc_HTTP_REQUEST_H_

#include "../Util/libft.h"
#include "multipart_data.h"

class HttpRequest {
 public:
  HttpRequest();
  ~HttpRequest();

  const std::string& GetMethod() const;
  const std::string& GetPath() const;
  const std::string& GetVersion() const;
  const std::string& GetQueryString() const;
  const std::string& GetBody() const;
  const std::string& GetContentType() const;
  const std::string& GetBoundary() const;
  const MultipartData& GetMultipartData() const;
  std::size_t GetContentLength() const;
  bool GetKeepAlive() const;
  bool GetIsChunked() const;
  const std::map<std::string, std::string>& GetHeaders() const;
  int GetPort() const;

  void SetMethod(const std::string& method);
  void SetPath(const std::string& path);
  void SetVersion(const std::string& version);
  void SetQueryString(const std::string& query_string);
  void SetBody(const std::string& body);
  void SetContentType(const std::string& content_type);
  void SetBoundary(const std::string& boundary);
  void SetContentLength(const std::size_t content_length);
  void SetKeepAlive(const bool keep_alive);
  void SetIsChunked(const bool is_chunked);
  void SetHeaders(const std::map<std::string, std::string>& headers);
  void SetHeader(const std::string& key, const std::string& value);
  void SetMultipartData(const MultipartData& data);
  void SetPort(int port);

  bool IsMultipart() const;
  void Reset();

 private:
  std::string method_;
  std::string path_;
  std::string version_;
  std::string query_string_;
  std::map<std::string, std::string> headers_;
  std::string body_;
  MultipartData multipart_data_;
  bool keep_alive_;
  std::size_t content_length_;
  std::string content_type_;
  std::string boundary_;
  bool is_chunked_;
  int port_;
};

#endif
