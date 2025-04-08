#include "../../inc/Response/http_response.h"

HttpResponse::HttpResponse() : status_code_(200), clientFd_(-1), is_cgi_response_(false), is_cgi_processed_(false) {
}

HttpResponse::~HttpResponse() {
}

void HttpResponse::SetStatus(int code, const std::string& message) {
  status_code_ = code;
  status_message_ = message;
}

void HttpResponse::SetHeader(const std::string& key, const std::string& value) {
  headers_[libft::FT_ToLower(key)] = value;
}

void HttpResponse::SetBody(const std::string& body) { body_ = body; }

void HttpResponse::SetClientFd(int fd)
{
    clientFd_ = fd;
}

int HttpResponse::GetClientFd() const
{
    return clientFd_;
}

void HttpResponse::SetIsCgiResponse(bool is_cgi_response) {
  is_cgi_response_ = is_cgi_response;
}

bool HttpResponse::GetIsCgiResponse() const {
  return is_cgi_response_;
}

void HttpResponse::SetIsCgiProcessed(bool is_cgi_processed) {
  is_cgi_processed_ = is_cgi_processed;
}

bool HttpResponse::GetIsCgiProcessed() const {
  return is_cgi_processed_;
}

std::string HttpResponse::ToString() const {
  std::stringstream ss;

  ss << "HTTP/1.1 " << status_code_;
  if (!status_message_.empty()) {
    ss << " " << status_message_;
  }
  ss << "\r\n";

  for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
       it != headers_.end(); ++it) {
    ss << libft::FT_StrCapitalize(it->first) << ": " << it->second << "\r\n";
  }

  if (headers_.find("content-length") == headers_.end()) {
    ss << "Content-Length: " << body_.length() << "\r\n";
  }

  ss << "\r\n";

  ss << body_;

  return ss.str();
}

int HttpResponse::GetStatusCode() const { return status_code_; }

std::string HttpResponse::GetHeader(const std::string& key) const {
  std::map<std::string, std::string>::const_iterator it = headers_.find(libft::FT_ToLower(key));
  return (it != headers_.end()) ? it->second : "";
}

const std::map<std::string, std::string>& HttpResponse::GetHeaders() const {
  return headers_;
}

std::string HttpResponse::GetBody() const { return body_; }

const std::string& HttpResponse::GetStatusMessage() const {
  return status_message_;
}

void HttpResponse::Clear()
{
    status_code_ = 0;
    status_message_.clear();
    headers_.clear();
    body_.clear();
    is_cgi_response_ = false;
    is_cgi_processed_ = false;
}
