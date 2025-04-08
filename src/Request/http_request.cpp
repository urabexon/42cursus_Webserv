#include "../../inc/Request/http_request.h"

HttpRequest::HttpRequest()
    : method_(""),
      path_(""),
      version_(""),
      query_string_(""),
      body_(""),
      keep_alive_(false),
      content_length_(static_cast<std::size_t>(-1)),
      content_type_(""),
      boundary_(""),
      is_chunked_(false),
      port_(-1) {}

HttpRequest::~HttpRequest() {}

const std::string& HttpRequest::GetMethod() const { return method_; }

const std::string& HttpRequest::GetPath() const { return path_; }

const std::string& HttpRequest::GetVersion() const { return version_; }

const std::string& HttpRequest::GetQueryString() const { return query_string_; }

const std::string& HttpRequest::GetBody() const { return body_; }

const std::string& HttpRequest::GetContentType() const { return content_type_; }

const std::string& HttpRequest::GetBoundary() const { return boundary_; }

std::size_t HttpRequest::GetContentLength() const { return content_length_; }

bool HttpRequest::GetKeepAlive() const { return keep_alive_; }

bool HttpRequest::GetIsChunked() const { return is_chunked_; }

const std::map<std::string, std::string>& HttpRequest::GetHeaders() const {
  return headers_;
}

void HttpRequest::SetMethod(const std::string& method) { method_ = method; }

void HttpRequest::SetPath(const std::string& path) { path_ = path; }

void HttpRequest::SetVersion(const std::string& version) { version_ = version; }

void HttpRequest::SetQueryString(const std::string& query_string) {
  query_string_ = query_string;
}

void HttpRequest::SetBody(const std::string& body) { body_ = body; }

void HttpRequest::SetContentType(const std::string& content_type) {
  content_type_ = content_type;
}

void HttpRequest::SetBoundary(const std::string& boundary) {
  boundary_ = boundary;
}

void HttpRequest::SetContentLength(const std::size_t content_length) {
  content_length_ = content_length;
}

void HttpRequest::SetKeepAlive(const bool keep_alive) {
  keep_alive_ = keep_alive;
}

void HttpRequest::SetIsChunked(const bool is_chunked) {
  is_chunked_ = is_chunked;
}

void HttpRequest::SetHeaders(
    const std::map<std::string, std::string>& headers) {
  headers_ = headers;
}

void HttpRequest::SetHeader(const std::string& key, const std::string& value) {
  headers_[libft::FT_ToLower(key)] = value;
}

bool HttpRequest::IsMultipart() const {
  return content_type_.find("multipart/form-data") != std::string::npos;
}

void HttpRequest::SetMultipartData(const MultipartData& data) {
  multipart_data_ = data;
}

const MultipartData& HttpRequest::GetMultipartData() const {
  return multipart_data_;
}

int HttpRequest::GetPort() const { return port_; }

void HttpRequest::SetPort(int port) { port_ = port; }

void HttpRequest::Reset() {
  method_ = "";
  path_ = "";
  version_ = "";
  query_string_ = "";
  headers_.clear();
  body_ = "";
  multipart_data_ = MultipartData();
  keep_alive_ = false;
  content_length_ = static_cast<std::size_t>(-1);
  content_type_ = "";
  boundary_ = "";
  is_chunked_ = false;
  port_ = -1;
}
