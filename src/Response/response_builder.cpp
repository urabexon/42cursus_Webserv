#include "../../inc/Response/response_builder.h"
#include "../../inc/Web/client_connection.h"


ResponseBuilder::ResponseBuilder(ServerConfig *config)
    : config_(config)
{
  if (!config_)
  {
    throw std::runtime_error("No server configuration provided");
  }
}

ResponseBuilder::~ResponseBuilder() {}

ServerConfig* ResponseBuilder::GetConfig() {
  return config_;
}

void ResponseBuilder::SetConfig(ServerConfig* config) {
  if (!config) {
    throw std::runtime_error("Invalid server configuration");
  }
  config_ = config;
}

const HttpConfig* ResponseBuilder::GetHttpConfig() const {
  if (!config_) {
    return NULL;
  }
  return static_cast<const HttpConfig*>(config_->GetHttpConfig());
}

void ResponseBuilder::ExecuteRequest(const HttpRequest &request)
{
    if (HandleRedirect(request, response_, *config_))
    {
      return;
    }

    const LocationConfig *location = FindMatchingLocation(request, config_);
    if (!location)
    {
      throw NotFoundException();
    }

    ValidateHttpMethod(request.GetMethod(), location);
    HandleHttpMethod(request);
}

void ResponseBuilder::HandleHttpMethod(const HttpRequest &request)
{
  if (request.GetMethod() == "GET")
  {
    HandleGetRequest(request, response_, *config_);
  }
  else if (request.GetMethod() == "POST")
  {
    HandlePostRequest(request, response_, *config_);
  }
  else if (request.GetMethod() == "DELETE")
  {
    HandleDeleteRequest(request, response_, *config_);
  }
}

void ResponseBuilder::ValidateHttpMethod(const std::string &method,
                                         const LocationConfig *location)
{
  if (!location)
  {
    throw std::runtime_error("Location is not set");
  }

  if (!IsMethodAccepted(method, location->GetAcceptedMethods()))
  {
    throw ForbiddenException();
  }
}

bool ResponseBuilder::IsMethodAccepted(const std::string &method,
                                      const std::vector<std::string> &accepted_methods)
{
  for (std::vector<std::string>::const_iterator it = accepted_methods.begin();
       it != accepted_methods.end(); ++it)
  {
    if (*it == method)
    {
      return true;
    }
  }
  return false;
}

std::string ResponseBuilder::ResolveRootPath(
    const std::string &location_root, const ServerConfig &config) const
{
  if (location_root.empty())
  {
    throw InternalServerErrorException();
  }

  std::string resolved_path = CombineRootPaths(location_root, config);
  ValidateResolvedPath(resolved_path);
  NormalizePathSeparators(resolved_path);
  ValidateDirectoryAccess(resolved_path);

  return resolved_path;
}

std::string ResponseBuilder::CombineRootPaths(
    const std::string &location_root, const ServerConfig &config) const
{
  if (location_root[0] != '/')
  {
    std::string server_root = config.GetRoot();
    if (server_root.empty())
    {
      throw InternalServerErrorException();
    }
    return server_root + "/" + location_root;
  }
  else
  {
    return location_root;
  }
}

void ResponseBuilder::ValidateResolvedPath(const std::string &resolved_path) const
{
  if (resolved_path.find("..") != std::string::npos)
  {
    throw ForbiddenException();
  }
}

void ResponseBuilder::NormalizePathSeparators(std::string &resolved_path) const
{
  std::string::iterator it = resolved_path.begin();
  std::string::iterator next;
  while ((next = std::find(it, resolved_path.end(), '/')) != resolved_path.end())
  {
    it = next;
    while (++next != resolved_path.end() && *next == '/')
      ;
    if (next - it > 1)
    {
      resolved_path.erase(it + 1, next);
      it = resolved_path.begin();
    }
    else
    {
      ++it;
    }
  }
}

void ResponseBuilder::ValidateDirectoryAccess(const std::string &resolved_path) const
{
  struct stat path_stat;
  if (stat(resolved_path.c_str(), &path_stat) != 0)
  {
    if (errno == EACCES)
    {
      throw ForbiddenException();
    }
    throw NotFoundException();
  }

  if (!S_ISDIR(path_stat.st_mode))
  {
    throw InternalServerErrorException();
  }

  if (access(resolved_path.c_str(), R_OK) != 0)
  {
    throw ForbiddenException();
  }
}

std::string ResponseBuilder::ResolveFinalPath(const LocationConfig *location,
                                              const HttpRequest &request,
                                              const ServerConfig &config,
                                              bool *is_directory)
{
  std::string resolved_root = ResolveRootPath(location->GetRoot(), config);
  std::string remaining_path = ExtractRemainingPath(location, request);
  std::string final_path = CombinePaths(resolved_root, remaining_path);

  if (IsCgiPath(request, location))
  {
    if (is_directory != NULL)
    {
      *is_directory = false;
    }
    return final_path;
  }

  return ValidateAndCheckPath(final_path, is_directory);
}

std::string ResponseBuilder::ExtractRemainingPath(const LocationConfig *location,
                                                const HttpRequest &request) const
{
  std::string remaining_path = request.GetPath().substr(location->GetPath().length());

  if (!remaining_path.empty() && remaining_path[0] != '/')
  {
    remaining_path = "/" + remaining_path;
  }

  return remaining_path;
}

std::string ResponseBuilder::CombinePaths(const std::string &root_path,
                                        const std::string &remaining_path) const
{
  std::string normalized_root = root_path;

  if (!normalized_root.empty() && normalized_root[normalized_root.length() - 1] == '/')
  {
    normalized_root = normalized_root.substr(0, normalized_root.length() - 1);
  }

  return normalized_root + remaining_path;
}

bool ResponseBuilder::IsCgiPath(const HttpRequest &request,
                              const LocationConfig *location) const
{
  size_t lastDot = request.GetPath().find_last_of('.');
  if (lastDot != std::string::npos)
  {
    std::string extension = request.GetPath().substr(lastDot);
    if (!location->GetCgiExecutor(extension).empty())
    {
      return true;
    }
  }

  return false;
}

std::string ResponseBuilder::ValidateAndCheckPath(const std::string &path, bool *is_directory) const
{
  struct stat file_stat;
  if (stat(path.c_str(), &file_stat) != 0)
  {
    throw NotFoundException();
  }

  bool is_dir = S_ISDIR(file_stat.st_mode);
  if (is_directory != NULL)
  {
    *is_directory = is_dir;
  }

  return path;
}

void ResponseBuilder::HandleGetRequest(const HttpRequest &request,
                                       HttpResponse *response,
                                       const ServerConfig &config)
{
  const LocationConfig *location = FindMatchingLocation(request, &config);
  bool is_directory;
  std::string final_path = ResolveFinalPath(location, request, config, &is_directory);

  if (ShouldHandleAsCgi(request, final_path, location))
  {
    HandleCgiRequest(request, response, *location, final_path);
    return;
  }

  if (is_directory)
  {
    if (TryFindIndexFile(final_path, location, &final_path))
    {
      is_directory = false;
    }
    else if (location->GetAutoindex())
    {
      ServeDirectoryListing(final_path, request.GetPath(), response);
      return;
    }
    else
    {
      throw ForbiddenException();
    }
  }

  ServeRegularFile(final_path, response);
}

bool ResponseBuilder::ShouldHandleAsCgi(const HttpRequest &request,
                                      const std::string &path,
                                      const LocationConfig *location)
{
  std::string request_path = request.GetPath();
  size_t lastDot = request_path.find_last_of('.');

  if (lastDot != std::string::npos)
  {
    std::string extension = request_path.substr(lastDot);
    if (!location->GetCgiExecutor(extension).empty())
    {
      return true;
    }
  }

  if (!location->GetScriptFilename().empty() &&
      path.find(".php") != std::string::npos)
  {
    return true;
  }

  return false;
}

bool ResponseBuilder::TryFindIndexFile(const std::string &path,
                                     const LocationConfig *location,
                                     std::string *index_path)
{
  if (!location)
  {
    return false;
  }

  const std::vector<std::string> &index_files = location->GetIndexFiles();
  for (std::vector<std::string>::const_iterator it = index_files.begin();
       it != index_files.end(); ++it)
  {
    std::string index_path_str = path;
    if (index_path_str[index_path_str.length() - 1] != '/')
    {
      index_path_str += "/";
    }
    index_path_str += *it;

    struct stat file_stat;
    if (stat(index_path_str.c_str(), &file_stat) == 0 &&
        S_ISREG(file_stat.st_mode))
    {
      *index_path = index_path_str;
      return true;
    }
  }

  return false;
}

void ResponseBuilder::ServeDirectoryListing(const std::string &dir_path,
                                          const std::string &request_path,
                                          HttpResponse *response) const
{
  std::string listing = GenerateDirectoryListing(dir_path, request_path);
  response->SetStatus(200, "OK");
  response->SetHeader("Content-Type", "text/html");
  response->SetBody(listing);
}

std::string ResponseBuilder::CreateDirectoryHeader(const std::string &request_path) const
{
  std::stringstream header;
  header << "<html>\n"
         << "<head><title>Index of " << request_path << "</title></head>\n"
         << "<body>\n"
         << "<h1>Index of " << request_path
         << "</h1><hr><pre><a href=\"../\">../</a>\n";
  return header.str();
}

std::string ResponseBuilder::CreateDirectoryEntry(const std::string &entry_name,
                                                const struct stat &file_stat) const
{
  std::time_t mod_time = file_stat.st_mtime;
  std::tm *tm_mod_time = std::localtime(&mod_time);

  char time_str[20];
  std::strftime(time_str, sizeof(time_str), "%d-%b-%Y %H:%M", tm_mod_time);

  std::stringstream size_stream;
  size_stream << file_stat.st_size;

  std::stringstream entry;
  entry << "<a href=\"" << entry_name << "\">" << entry_name
        << "</a>" << std::string(50 - entry_name.length(), ' ')
        << time_str << " " << std::setw(10) << size_stream.str()
        << "\n";

  return entry.str();
}

std::string ResponseBuilder::GenerateDirectoryListing(
    const std::string &path, const std::string &request_path) const
{
  std::stringstream listing;
  listing << CreateDirectoryHeader(request_path);

  DIR *dir = opendir(path.c_str());
  if (dir)
  {
    ReadDirectoryEntries(dir, path, listing);
    closedir(dir);
  }

  listing << "</pre><hr></body>\n"
          << "</html>";
  return listing.str();
}

void ResponseBuilder::ReadDirectoryEntries(DIR *dir,
                                         const std::string &path,
                                         std::stringstream &listing) const
{
  struct dirent *entry;
  while ((entry = readdir(dir)))
  {
    if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
    {
      continue;
    }

    std::string full_path = path + "/" + entry->d_name;
    struct stat file_stat;
    if (stat(full_path.c_str(), &file_stat) == 0)
    {
      listing << CreateDirectoryEntry(entry->d_name, file_stat);
    }
  }
}

void ResponseBuilder::ServeRegularFile(const std::string &file_path,
                                     HttpResponse *response) const
{
  struct stat file_stat;
  if (stat(file_path.c_str(), &file_stat) != 0)
  {
    throw ForbiddenException();
  }

  if (!S_ISREG(file_stat.st_mode))
  {
    throw ForbiddenException();
  }

  std::ifstream file(file_path.c_str(), std::ios::binary);
  if (!file.good())
  {
    throw NotFoundException();
  }

  response->SetStatus(200, "OK");

  std::string extension = file_path.substr(file_path.find_last_of(".") + 1);
  std::string mimeType = MimeType::GetType(extension);
  response->SetHeader("Content-Type", mimeType);

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string body = buffer.str();
  response->SetBody(body);
}

void ResponseBuilder::HandlePostRequest(const HttpRequest &request,
                                        HttpResponse *response,
                                        const ServerConfig &config)
{
  const LocationConfig *location = FindMatchingLocation(request, &config);
  if (!location)
  {
    throw NotFoundException();
  }

  bool is_directory;
  std::string final_path = ResolveFinalPath(location, request, config, &is_directory);

  if (ShouldHandleAsCgi(request, final_path, location))
  {
    HandleCgiRequest(request, response, *location, final_path);
    return;
  }

  if (IsMultipartFormData(request))
  {
    HandleMultipartUpload(location, request);
    return;
  }
  else if (request.GetIsChunked())
  {
    HandleChunkedRequest(response);
    return;
  }

  throw MethodNotAllowedException();
}

bool ResponseBuilder::IsMultipartFormData(const HttpRequest &request) const
{
  std::map<std::string, std::string>::const_iterator content_type_it =
      request.GetHeaders().find("content-type");

  return (content_type_it != request.GetHeaders().end() &&
          content_type_it->second.find("multipart/form-data") != std::string::npos);
}

void ResponseBuilder::HandleMultipartUpload(const LocationConfig *location,
                                          const HttpRequest &request) const
{
  const MultipartData &data = request.GetMultipartData();
  const std::vector<FileUpload> &files = data.GetFiles();

  std::string upload_path = EnsureUploadDirectory(location);

  if (files.empty())
  {
    SetSuccessResponse(response_, upload_path, "Form data processed successfully");
    return;
  }

  if (SaveUploadedFiles(files, upload_path))
  {
    std::ostringstream oss;
    oss << "Files uploaded successfully: " << files.size() << " file(s)";
    SetSuccessResponse(response_, upload_path, oss.str());
  }
  else
  {
    throw InternalServerErrorException();
  }
}

std::string ResponseBuilder::EnsureUploadDirectory(const LocationConfig *location) const
{
  std::string upload_path = location->GetUploadPath();
  if (upload_path.empty())
  {
    throw InternalServerErrorException();
  }

  struct stat st;
  if (stat(upload_path.c_str(), &st) != 0)
  {
    throw InternalServerErrorException();
  }
  else if (!S_ISDIR(st.st_mode))
  {
    throw InternalServerErrorException();
  }

  return upload_path;
}

bool ResponseBuilder::SaveUploadedFiles(const std::vector<FileUpload> &files,
                                      const std::string &upload_path) const
{
  for (std::vector<FileUpload>::const_iterator it = files.begin();
       it != files.end(); ++it)
  {
    if (!it->SaveToFile(upload_path))
    {
      return false;
    }
  }

  return true;
}

void ResponseBuilder::SetSuccessResponse(HttpResponse *response,
                                       const std::string &location,
                                       const std::string &message) const
{
  response->SetStatus(201, "Created");
  response->SetHeader("Content-Type", "text/plain");
  response->SetHeader("Location", location);
  response->SetBody(message);
}

void ResponseBuilder::HandleChunkedRequest(HttpResponse *response) const
{
  response->SetStatus(200, "OK");
  response->SetHeader("Transfer-Encoding", "chunked");
}

void ResponseBuilder::HandleDeleteRequest(const HttpRequest &request,
                                          HttpResponse *response,
                                          const ServerConfig &config)
{
  const LocationConfig *location = FindMatchingLocation(request, &config);
  if (!location)
  {
    throw NotFoundException();
  }

  bool is_directory;
  std::string final_path = ResolveFinalPath(location, request, config, &is_directory);

  if (is_directory)
  {
    throw ForbiddenException();
  }

  DeleteFile(final_path);

  response->SetStatus(200, "OK");
  response->SetHeader("Content-Type", "text/plain");
  response->SetBody("File deleted successfully");
}

bool ResponseBuilder::DeleteFile(const std::string &path) const
{
  if (std::remove(path.c_str()) != 0)
  {
    HandleDeleteError();
  }
  return true;
}

void ResponseBuilder::HandleDeleteError() const
  {
    switch (errno)
    {
    case EACCES:
      throw ForbiddenException();
    case ENOENT:
      throw NotFoundException();
    default:
      throw InternalServerErrorException();
    }
}

bool ResponseBuilder::HandleRedirect(const HttpRequest &request,
                                     HttpResponse *response,
                                     const ServerConfig &config)
{
  const LocationConfig *location = FindMatchingLocation(request, &config);
  if (location)
  {
    const std::pair<std::string, int> &location_redirect = location->GetRedirect();
    if (!location_redirect.first.empty() && location_redirect.second != -1)
    {
      int status_code = location_redirect.second;
      response->SetStatus(status_code, getRedirectMessage(status_code));
      if (status_code == 301 || status_code == 302 || status_code == 303 || status_code == 307 || status_code == 308)
      {
        response->SetHeader(
            "Location",
            "http://" + request.GetHeaders().at("host") + location_redirect.first);
      }
      else
      {
        response->SetHeader(
            "Content-Type",
            "text/plain");
      }

      if (response->GetStatusMessage().empty())
      {
        response->SetBody(location_redirect.first);
      }
      else
      {
        BuildBody(status_code, config);
      }
      return true;
    }
  }
  return false;
}

std::string ResponseBuilder::getRedirectMessage(int code)
{
  switch (code)
  {
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 303:
    return "See Other";
  case 307:
    return "Temporary Redirect";
  case 308:
    return "Permanent Redirect";
  default:
    return "";
  }
}

void ResponseBuilder::BuildStatus(int status_code,
                                  const std::string &status_message)
{
  response_->SetStatus(status_code, status_message);
}

void ResponseBuilder::BuildHeaders(int status_code)
{
  if (status_code == 400 || status_code >= 500 || response_->GetHeader("Connection") == "close")
  {
    response_->SetHeader("Connection", "close");
  }
  else
  {
    response_->SetHeader("Connection", "keep-alive");
  }

  response_->SetHeader("Server", "johnx/1.0.0");
  response_->SetHeader("Date", libft::FT_GetGMTDate());
}

void ResponseBuilder::BuildBody(int status_code, const ServerConfig &config)
{
  const std::map<int, std::string> &error_pages = config.GetErrorPages();
  if (!error_pages.empty())
  {
    std::map<int, std::string>::const_iterator it = error_pages.find(status_code);
    if (it != error_pages.end())
    {
      std::string file_path = config.GetRoot();
      if (!file_path.empty() && file_path[file_path.length() - 1] != '/')
      {
        file_path += "/";
      }
      file_path += it->second;

      std::ifstream file(file_path.c_str());
      if (file.is_open())
      {
        std::stringstream buffer;
        buffer << file.rdbuf();
        response_->SetBody(buffer.str());

        std::string::size_type dot_pos = it->second.find_last_of(".");
        if (dot_pos != std::string::npos) {
          std::string extension = it->second.substr(dot_pos + 1);
          response_->SetHeader("Content-Type", MimeType::GetType(extension));
        } else {
          response_->SetHeader("Content-Type", MimeType::GetType(""));
        }

        file.close();
        return;
      }
    }
  }
  BuildDefaultErrorPage(status_code);
}

void ResponseBuilder::BuildDefaultErrorPage(int status_code)
{
  if (status_code >= 300)
  {
    std::stringstream body;
    body << "<html>\n"
         << "<head><title>" << status_code << " "
         << response_->GetStatusMessage() << "</title></head>\n"
         << "<body>\n"
         << "<center><h1>" << status_code << " "
         << response_->GetStatusMessage() << "</h1></center>\n"
         << "<hr><center>johnx/1.0.0</center>\n"
         << "</body>\n"
         << "</html>\n";
    response_->SetHeader("Content-Type", "text/html");
    response_->SetBody(body.str());
  }
}

HttpResponse *ResponseBuilder::GetResponse()
{
  return response_;
}

void ResponseBuilder::SetResponse(HttpResponse *response)
{
  response_ = response;
}

ResponseDirector::ResponseDirector(ResponseBuilder *builder)
    : builder_(builder) {}

ResponseDirector::~ResponseDirector() {}

void ResponseDirector::ConstructResponse(const HttpRequest &request)
{
    try
    {
        builder_->ExecuteRequest(request);
        builder_->BuildHeaders(builder_->GetResponse()->GetStatusCode());
    }
    catch (const HttpException &e)
    {
        ConstructErrorResponse(e.getStatus(), e.what());
    }
    catch (const std::exception &e)
    {
        ConstructErrorResponse(500, e.what());
    }
}

void ResponseDirector::ConstructErrorResponse(int status_code,
                                              const std::string &message)
{
  try
  {
    builder_->GetResponse()->SetStatus(status_code, message);
    builder_->BuildHeaders(status_code);
    builder_->BuildBody(status_code, *builder_->GetConfig());
  }
  catch (const std::exception &e)
  {
    builder_->GetResponse()->SetStatus(500, "Internal Server Error");
  }
}

HttpResponse *ResponseDirector::GetResponse()
{
  return builder_->GetResponse();
}

void ResponseDirector::SetResponse(HttpResponse *response)
{
  builder_->SetResponse(response);
}

void ResponseDirector::SetClientFd(int fd)
{
  builder_->SetClientFd(fd);
}

void ResponseBuilder::HandleCgiRequest(const HttpRequest &request,
                                       HttpResponse *response,
                                       const LocationConfig &location,
                                       const std::string &scriptPath)
{
  std::string executor = GetCgiExecutor(request, location);
  ValidateScriptPath(scriptPath);

  ClientConnection* client = GetClientConnection(response);
  CgiHandler *cgi = client->generateCgiHandler();

  SetupCgiHandler(cgi, response, executor, response->GetClientFd(), location);
  cgi->executeCgi(*config_, request, scriptPath);

  client->SetCgiPid(cgi->getChildPid());

  RegisterCgiHandler(cgi);
}

std::string ResponseBuilder::GetCgiExecutor(const HttpRequest &request,
                                          const LocationConfig &location) const
{
  std::string path = request.GetPath();
  size_t lastDot = path.find_last_of('.');
  if (lastDot == std::string::npos)
  {
    throw HttpException(400, "Bad Request");
  }

  std::string extension = path.substr(lastDot);
  std::string executor = location.GetCgiExecutor(extension);

  if (executor.empty())
  {
    throw HttpException(500, "Internal Server Error");
  }

  return executor;
}

void ResponseBuilder::ValidateScriptPath(const std::string &scriptPath) const
{
  if (access(scriptPath.c_str(), F_OK) == -1)
  {
    throw HttpException(404, "Not Found");
  }
}

ClientConnection* ResponseBuilder::GetClientConnection(HttpResponse *response) const
{
  int clientFd = response->GetClientFd();

  if (clientFd == -1) {
    throw HttpException(500, "Internal Server Error");
  }

  ClientConnection* client = EpollHandler::Instance().FindClientByFd(clientFd);
  if (!client) {
    throw HttpException(500, "Internal Server Error - Client not found");
  }

  return client;
}

void ResponseBuilder::SetupCgiHandler(CgiHandler *cgi,
                                    HttpResponse *response,
                                    const std::string &executor,
                                    int clientFd,
                                    const LocationConfig &location)
{
  cgi->setResponse(response);
  cgi->setExecutor(executor);
  cgi->setClientFd(clientFd);
  cgi->setTimeout(location.GetCgiReadTimeout());

  response->SetIsCgiResponse(true);
  response->SetIsCgiProcessed(false);
}

void ResponseBuilder::RegisterCgiHandler(CgiHandler *cgi) const
{
  if (!EpollHandler::Instance().RegisterEvent(cgi, EPOLLIN | EPOLLRDHUP | EPOLLHUP))
  {
    throw std::runtime_error("Failed to register CGI handler with EpollHandler");
  }

  cgi->setRegistered(true);
}

void ResponseBuilder::SetClientFd(int fd)
{
  response_->SetClientFd(fd);
}
