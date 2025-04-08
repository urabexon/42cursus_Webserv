#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <iostream>

#include "../Response/http_response.h"
#include "../Response/response_builder.h"
#include "../Cgi/cgi_handler.h"
#include "../Web/epoll_handler.h"
#include "../Exception/http_exception.h"
#include "../Request/request_parser.h"

#define BUFFER_SIZE 4096
#define CRLF "\r\n"
#define CTRL_C_SEQUENCE "\xff\xf4\xff\xfd\x06"
#define CTRL_Z_SEQUENCE "\xff\xed\xff\xfd\x06"
#define CTRL_BACKSLASH_SEQUENCE "\xff\xf3\xff\xfd\x06"
#define SEQUENCE_LEN 5

class ResponseBuilder;
class ResponseDirector;

class ClientConnection : public Event {
 public:
  ClientConnection(int fd, ServerConfig* config);
  ~ClientConnection();

  void OnEvent(uint32_t events);
  int getFd() const;

  bool IsClosed() const;
  void Close();
  bool IsTimedOut() const;
  bool IsCGITimeout() const;
  bool IsCgi() const;
  void setCgiHandler(CgiHandler* handler);
  CgiHandler* getCgiHandler() const;
  CgiHandler* generateCgiHandler();
  void handleCgiResponse(const std::string& response);

  void KillCgiProcess();
  void SetCgiPid(pid_t pid);
  pid_t GetCgiPid() const;

  void MarkForDeletion();
  bool ShouldDelete() const;

 private:
  void HandleRead();
  void HandleWrite();
  void HandleClose();
  void HandleClientRequest(const HttpRequest& request);
  void HandleParsingException(const HttpException& e);
  void SetupResponseForSending();
  void UpdateActivity();

  ssize_t ReadDataFromClient();
  bool CheckForControlSequences(char* buf, ssize_t n);
  bool CheckForInvalidRequest(char* buf, ssize_t n, ssize_t total_read);
  void SendBadRequestResponse();
  void ProcessReadBuffer(bool& is_connection_close);
  void SetupRequestPort(HttpRequest& req);

  void UpdateServerConfig(const HttpRequest &request);
  void UpdateTimeouts(const HttpRequest &request);
  bool CheckConnectionCloseHeader(const HttpRequest &request);

  void HandleCgiTimeout();
  void WriteResponseData();
  void HandleEmptyWriteBuffer();

  void HandleEmptyCgiResponse();
  void HandleErrorCgiResponse(int status);
  void ParseCgiHeaderAndBody(const std::string &response, std::string::size_type headerEnd);
  void ParseCgiHeaders(const std::string &headerPart);
  bool ParseStatusHeader(const std::string &line);
  void ParseNormalHeader(const std::string &line);
  void HandleCgiResponseWithoutHeaderEnd(const std::string &response);
  void FinalizeCgiResponse();

 private:
  int fd_;
  bool closed_;
  bool should_close_;
  bool should_delete_;
  RequestParser* parser_;
  ResponseBuilder* builder_;
  HttpResponse* response_;
  ResponseDirector* director_;

  std::string read_buffer_;
  std::string write_buffer_;
  std::time_t last_activity_;
  std::time_t keepalive_timeout_;

  CgiHandler* cgi_handler_;
  std::time_t cgi_read_timeout_;
  pid_t cgi_pid_;
};

#endif
