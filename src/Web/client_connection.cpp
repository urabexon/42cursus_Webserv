#include "../../inc/Web/client_connection.h"

ClientConnection::ClientConnection(int fd, ServerConfig *config)
    : fd_(fd), closed_(false), should_close_(false), should_delete_(false), keepalive_timeout_(60000), cgi_handler_(NULL), cgi_read_timeout_(60000), cgi_pid_(-1)
{
  if (fcntl(fd_, F_SETFL, O_NONBLOCK) < 0)
  {
    throw std::runtime_error("Failed to set non-blocking");
  }
  parser_ = new RequestParser(config);
  builder_ = new ResponseBuilder(config);
  director_ = new ResponseDirector(builder_);
  response_ = new HttpResponse();
  director_->SetResponse(response_);

  UpdateActivity();
}

ClientConnection::~ClientConnection()
{
  if (!closed_) {
    Close();
  }

  delete parser_;
  delete builder_;
  delete director_;
  delete response_;

  if (cgi_handler_ != NULL) {
    delete cgi_handler_;
    cgi_handler_ = NULL;
  }
}

void ClientConnection::OnEvent(uint32_t events)
{
  if (events & EPOLLIN)
  {
    HandleRead();
  }
  if (events & EPOLLOUT)
  {
    HandleWrite();
  }
  if (events & (EPOLLRDHUP | EPOLLERR))
  {
    HandleClose();
  }
}

void ClientConnection::HandleClose()
{
  if (!closed_)
  {
    Close();
  }
}

int ClientConnection::getFd() const
{
  return fd_;
}

bool ClientConnection::IsClosed() const { return closed_; }

void ClientConnection::HandleRead()
{
  if (closed_)
    return;
  UpdateActivity();

  bool is_connection_close = false;
  ssize_t total_read = ReadDataFromClient();

  if (total_read < 0)
    return;

  if (total_read == 0 && read_buffer_.empty())
    return;

  try
  {
    ProcessReadBuffer(is_connection_close);
  }
  catch (const HttpException &e)
  {
    HandleParsingException(e);
  }

  if (is_connection_close)
  {
    should_close_ = true;
  }
}

ssize_t ClientConnection::ReadDataFromClient()
{
  char buf[4096];
  ssize_t total_read = 0;

  while (true)
  {
    ssize_t n = read(fd_, buf, sizeof(buf));
    if (n > 0)
    {
      total_read += n;

      if (CheckForControlSequences(buf, n))
        return -1;

      if (CheckForInvalidRequest(buf, n, total_read))
        return -1;

      read_buffer_.append(buf, n);
    }
    else if (n == 0)
    {
      if (IsCgi() && !response_->GetIsCgiProcessed())
      {
        break;
      }
      Close();
      return -1;
    }
    else
    {
      break;
    }
  }

  return total_read;
}

bool ClientConnection::CheckForControlSequences(char* buf, ssize_t n)
{
  if (n >= SEQUENCE_LEN && (std::string(buf, 5) == std::string(CTRL_C_SEQUENCE) ||
                            std::string(buf, 5) == std::string(CTRL_Z_SEQUENCE) ||
                            std::string(buf, 5) == std::string(CTRL_BACKSLASH_SEQUENCE)))
  {
    Close();
    return true;
  }

  return false;
}

bool ClientConnection::CheckForInvalidRequest(char* buf, ssize_t n, ssize_t total_read)
{
  if ((n == 1 && buf[0] == 4) || (total_read < BUFFER_SIZE && std::string(buf, n).find("\r\n") == std::string::npos))
  {
    SendBadRequestResponse();
    return true;
  }

  return false;
}

void ClientConnection::SendBadRequestResponse()
{
  director_->ConstructErrorResponse(400, "Bad Request");
  director_->GetResponse()->SetHeader("Connection", "close");
  write_buffer_ = director_->GetResponse()->ToString();
  EpollHandler::Instance().UpdateEvent(this, EPOLLOUT);
  parser_->Reset();
  response_->Clear();
  should_close_ = true;
}

void ClientConnection::ProcessReadBuffer(bool& is_connection_close)
{
  RequestParser::ParsingStatus status = parser_->Consume(read_buffer_);
  read_buffer_.clear();

  if (status == RequestParser::PARSE_COMPLETE)
  {
    HttpRequest req = parser_->GetRequest();
    SetupRequestPort(req);

    is_connection_close = CheckConnectionCloseHeader(req);
    if (is_connection_close)
    {
      should_close_ = true;
    }

    HandleClientRequest(req);
  }
}

void ClientConnection::SetupRequestPort(HttpRequest& req)
{
  int connected_port = -1;

  if (parser_->GetServer() && !parser_->GetServer()->GetListenDirectives().empty()) {
    connected_port = parser_->GetServer()->GetListenDirectives()[0].port;
  } else {
    connected_port = 8081;
  }

  req.SetPort(connected_port);
}

void ClientConnection::HandleClientRequest(const HttpRequest &request)
{
  if (IsCgi() && cgi_handler_ && !cgi_handler_->isComplete() && !response_->GetIsCgiProcessed()) {
    return;
  }

  UpdateServerConfig(request);
  director_->SetClientFd(fd_);
  director_->ConstructResponse(request);
  UpdateTimeouts(request);

  if (should_close_) {
    director_->GetResponse()->SetHeader("Connection", "close");
  }

  SetupResponseForSending();
}

void ClientConnection::UpdateServerConfig(const HttpRequest &request)
{
  const HttpConfig* http_config = builder_->GetConfig()->GetHttpConfig();
  if (http_config) {
    ServerConfig* matched_server = FindMatchingServerConfig(request, http_config);
    if (matched_server && matched_server != builder_->GetConfig()) {
      builder_->SetConfig(matched_server);
      parser_->SetConfig(matched_server);
    }
  }
}

void ClientConnection::UpdateTimeouts(const HttpRequest &request)
{
  const LocationConfig *location = FindMatchingLocation(request, builder_->GetConfig());
  if (location)
  {
    if (IsCgi())
    {
      cgi_read_timeout_ = location->GetCgiReadTimeout();
    }
    keepalive_timeout_ = location->GetKeepaliveTimeout();
  }
}

bool ClientConnection::CheckConnectionCloseHeader(const HttpRequest &request)
{
  bool has_close_header = false;
  const std::map<std::string, std::string> &headers = request.GetHeaders();
  std::map<std::string, std::string>::const_iterator it = headers.find("connection");
  if (it != headers.end() && libft::FT_ToLower(it->second) == "close")
  {
    has_close_header = true;
  }

  return (has_close_header || response_->GetHeader("connection") == "close");
}

void ClientConnection::HandleParsingException(const HttpException &e)
{
  director_->ConstructErrorResponse(e.getStatus(), e.what());
  director_->GetResponse()->SetHeader("Connection", "close");
  should_close_ = true;
  SetupResponseForSending();
  parser_->Reset();
  response_->Clear();
}

void ClientConnection::SetupResponseForSending()
{
  write_buffer_ = director_->GetResponse()->ToString();
  EpollHandler::Instance().UpdateEvent(this, EPOLLOUT);
  parser_->Reset();
}

void ClientConnection::HandleWrite()
{
  if (closed_)
    return;

  if(IsCgi() && !response_->GetIsCgiProcessed())
  {
    HandleCgiTimeout();
    return;
  }

  UpdateActivity();
  WriteResponseData();

  if (write_buffer_.empty())
  {
    HandleEmptyWriteBuffer();
  }
}

void ClientConnection::HandleCgiTimeout()
{
  if (IsCGITimeout())
  {
    KillCgiProcess();
    response_->SetStatus(504, "Gateway Timeout");
    response_->SetIsCgiProcessed(true);

    setCgiHandler(NULL);

    director_->ConstructErrorResponse(504, "Gateway Timeout");
    write_buffer_ = response_->ToString();
    EpollHandler::Instance().UpdateEvent(this, EPOLLOUT);
  }
}

void ClientConnection::WriteResponseData()
{
  while (!write_buffer_.empty())
  {
    ssize_t n = write(fd_, write_buffer_.c_str(), write_buffer_.size());
    if (n > 0)
    {
      write_buffer_.erase(0, n);
    }
    else
    {
      break;
    }
  }
}

void ClientConnection::HandleEmptyWriteBuffer()
{
  if (should_close_)
  {
    Close();
    return;
  }
  else
  {
    EpollHandler::Instance().UpdateEvent(this, EPOLLIN);
  }
  response_->Clear();
}

void ClientConnection::Close()
{
  if (!closed_)
  {
    closed_ = true;

    if (cgi_handler_ != NULL) {
      if (cgi_handler_->isRegisteredToEpoll()) {
        EpollHandler::Instance().UnregisterEvent(cgi_handler_);
        cgi_handler_->setRegistered(false);
      }

      EpollHandler::Instance().InvalidateEvent(cgi_handler_);
      EpollHandler::Instance().ScheduleForDeletion(cgi_handler_);
      cgi_handler_ = NULL;
    }

    EpollHandler::Instance().UnregisterEvent(this);

    if (fd_ >= 0) {
      close(fd_);
      fd_ = -1;
    }

    read_buffer_.clear();
    write_buffer_.clear();
    response_->Clear();

    should_close_ = false;

    MarkForDeletion();
  }
}

void ClientConnection::UpdateActivity()
{
  last_activity_ = std::time(NULL);
}

bool ClientConnection::IsTimedOut() const
{
  return std::time(NULL) - last_activity_ > keepalive_timeout_ / 1000;
}

bool ClientConnection::IsCgi() const
{
  return cgi_handler_ != NULL;
}

bool ClientConnection::IsCGITimeout() const
{
  if (!IsCgi() || cgi_handler_ == NULL) {
    return false;
  }

  try {
    return cgi_handler_->isTimedOut();
  } catch (const std::exception& e) {
    return false;
  }
}

void ClientConnection::handleCgiResponse(const std::string &response)
{
  if (response.empty())
  {
    HandleEmptyCgiResponse();
    return;
  }

  int currentStatus = response_->GetStatusCode();
  if (currentStatus == 500 || currentStatus == 504)
  {
    HandleErrorCgiResponse(currentStatus);
    return;
  }

  std::string::size_type headerEnd = response.find("\r\n\r\n");
  if (headerEnd != std::string::npos)
  {
    ParseCgiHeaderAndBody(response, headerEnd);
  }
  else
  {
    HandleCgiResponseWithoutHeaderEnd(response);
  }

  FinalizeCgiResponse();
}

void ClientConnection::HandleEmptyCgiResponse()
{
  response_->SetStatus(500, "Internal Server Error");
  response_->SetIsCgiProcessed(true);
  director_->ConstructErrorResponse(500, "Internal Server Error");
  write_buffer_ = response_->ToString();
  EpollHandler::Instance().UpdateEvent(this, EPOLLOUT);
}

void ClientConnection::HandleErrorCgiResponse(int status)
{
  director_->ConstructErrorResponse(status, response_->GetStatusMessage());
  response_->SetHeader("Connection", "close");
  write_buffer_ = response_->ToString();
  EpollHandler::Instance().UpdateEvent(this, EPOLLOUT);
}

void ClientConnection::ParseCgiHeaderAndBody(const std::string &response, std::string::size_type headerEnd)
{
  std::string headerPart = response.substr(0, headerEnd);
  std::string bodyPart = response.substr(headerEnd + 4);

  ParseCgiHeaders(headerPart);
  response_->SetBody(bodyPart);
}

void ClientConnection::ParseCgiHeaders(const std::string &headerPart)
{
  std::istringstream headerStream(headerPart);
  std::string line;
  bool statusSet = false;

  while (std::getline(headerStream, line))
  {
    if (!line.empty() && line[line.length() - 1] == '\r')
    {
      line.erase(line.length() - 1);
    }

    if (line.empty())
      continue;

    if (line.substr(0, 7) == "Status:")
    {
      statusSet = ParseStatusHeader(line);
    }
    else
    {
      ParseNormalHeader(line);
    }
  }

  if (!statusSet)
  {
    response_->SetStatus(200, "OK");
  }
}

bool ClientConnection::ParseStatusHeader(const std::string &line)
{
  std::string statusValue = line.substr(7);
  while (!statusValue.empty() && statusValue[0] == ' ')
    statusValue.erase(0, 1);

  std::string::size_type spacePos = statusValue.find(' ');
  int statusCode = 200;
  std::string statusMessage = "OK";

  if (spacePos != std::string::npos)
  {
    statusCode = std::atoi(statusValue.substr(0, spacePos).c_str());
    statusMessage = statusValue.substr(spacePos + 1);
  }
  else
  {
    statusCode = std::atoi(statusValue.c_str());
  }

  if (statusCode <= 0 || statusCode >= 600)
  {
    statusCode = 500;
    statusMessage = "Internal Server Error";
  }

  response_->SetStatus(statusCode, statusMessage);
  return true;
}

void ClientConnection::ParseNormalHeader(const std::string &line)
{
  std::string::size_type colonPos = line.find(':');
  if (colonPos != std::string::npos)
  {
    std::string headerName = line.substr(0, colonPos);
    std::string headerValue = line.substr(colonPos + 1);

    while (!headerValue.empty() && headerValue[0] == ' ')
      headerValue.erase(0, 1);

    response_->SetHeader(headerName, headerValue);
  }
}

void ClientConnection::HandleCgiResponseWithoutHeaderEnd(const std::string &response)
{
  if (response.find("<h1>500 Internal Server Error</h1>") != std::string::npos) {
    response_->SetStatus(500, "Internal Server Error");
    response_->SetIsCgiProcessed(true);
    director_->ConstructErrorResponse(500, "Internal Server Error");
  } else if (response.find("<h1>504 Gateway Timeout</h1>") != std::string::npos) {
    response_->SetStatus(504, "Gateway Timeout");
    response_->SetIsCgiProcessed(true);
    director_->ConstructErrorResponse(504, "Gateway Timeout");
  } else {
    response_->SetStatus(200, "OK");
    response_->SetHeader("Content-Type", "text/html");
    response_->SetBody(response);
  }
}

void ClientConnection::FinalizeCgiResponse()
{
  response_->SetIsCgiProcessed(true);
  write_buffer_ = response_->ToString();
  EpollHandler::Instance().UpdateEvent(this, EPOLLOUT);
}

void ClientConnection::KillCgiProcess()
{
  if (cgi_pid_ > 0)
  {
    kill(cgi_pid_, SIGTERM);
    cgi_pid_ = -1;
  }

  if (cgi_handler_) {
    if (cgi_handler_->isRegisteredToEpoll()) {
      EpollHandler::Instance().UnregisterEvent(cgi_handler_);
      cgi_handler_->setRegistered(false);
    }

    EpollHandler::Instance().InvalidateEvent(cgi_handler_);
    EpollHandler::Instance().ScheduleForDeletion(cgi_handler_);
    cgi_handler_ = NULL;
  }
}

CgiHandler* ClientConnection::generateCgiHandler()
{
  KillCgiProcess();

  CgiHandler* handler = new CgiHandler();
  setCgiHandler(handler);
  return handler;
}

void ClientConnection::setCgiHandler(CgiHandler* handler)
{
  cgi_handler_ = handler;
  if (handler) {
    handler->setClientFd(fd_);
    handler->setResponse(response_);
    const LocationConfig *location = FindMatchingLocation(parser_->GetRequest(), builder_->GetConfig());
    if (location) {
      cgi_read_timeout_ = location->GetCgiReadTimeout();
    }
    handler->setTimeout(cgi_read_timeout_);
  }
}

CgiHandler* ClientConnection::getCgiHandler() const
{
  return cgi_handler_;
}

void ClientConnection::SetCgiPid(pid_t pid)
{
  cgi_pid_ = pid;
}

pid_t ClientConnection::GetCgiPid() const
{
  return cgi_pid_;
}

void ClientConnection::MarkForDeletion()
{
  should_delete_ = true;
}

bool ClientConnection::ShouldDelete() const
{
  return should_delete_;
}
