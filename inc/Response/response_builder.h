#ifndef RESPONSE_BUILDER_H_
#define RESPONSE_BUILDER_H_

#include <dirent.h>
#include <sys/stat.h>
#include <iomanip>

#include "../Config/server_config.h"
#include "../Config/location_config.h"
#include "../Request/http_request.h"
#include "../Cgi/cgi_handler.h"
#include "mime_type.h"

class ClientConnection;
class CgiHandler;

class ResponseBuilder {
 public:
  ResponseBuilder(ServerConfig* config);
  ~ResponseBuilder();

  void ExecuteRequest(const HttpRequest& request);
  void BuildStatus(int status_code, const std::string& status_message);
  void BuildHeaders(int status_code);
  void BuildBody(int status_code, const ServerConfig& config);
  void BuildDefaultErrorPage(int status_code);

  HttpResponse* GetResponse();
  void SetResponse(HttpResponse* response);
  ServerConfig* GetConfig();
  void SetConfig(ServerConfig* config);
  const HttpConfig* GetHttpConfig() const;
  void SetClientFd(int fd);

  std::string getRedirectMessage(int code);

  void HandleDeleteError() const;
  std::string GenerateDirectoryListing(const std::string& path,
                                     const std::string& request_path) const;
  std::string CreateDirectoryHeader(const std::string &request_path) const;
  std::string CreateDirectoryEntry(const std::string &entry_name, const struct stat &file_stat) const;
  void ReadDirectoryEntries(DIR *dir, const std::string &path, std::stringstream &listing) const;
  bool HandleRedirect(const HttpRequest& request,
                     HttpResponse* response,
                     const ServerConfig& config);
  void HandleCgiRequest(const HttpRequest& request,
                       HttpResponse* response,
                       const LocationConfig& location,
                       const std::string& scriptPath);
  std::string GetCgiExecutor(const HttpRequest &request, const LocationConfig &location) const;
  void ValidateScriptPath(const std::string &scriptPath) const;
  ClientConnection* GetClientConnection(HttpResponse *response) const;
  void SetupCgiHandler(CgiHandler *cgi, HttpResponse *response, const std::string &executor,
                     int clientFd, const LocationConfig &location);
  void RegisterCgiHandler(CgiHandler *cgi) const;

 private:
  void HandleHttpMethod(const HttpRequest& request);
  void ValidateHttpMethod(const std::string& method, const LocationConfig* location);
  bool IsMethodAccepted(const std::string& method,
                       const std::vector<std::string>& accepted_methods);
  std::string ResolveRootPath(const std::string& location_root,
                             const ServerConfig& config) const;
  std::string CombineRootPaths(const std::string& location_root,
                              const ServerConfig& config) const;
  void ValidateResolvedPath(const std::string& resolved_path) const;
  void NormalizePathSeparators(std::string& resolved_path) const;
  void ValidateDirectoryAccess(const std::string& resolved_path) const;
  std::string ResolveFinalPath(const LocationConfig* location,
                              const HttpRequest& request,
                              const ServerConfig& config,
                              bool* is_directory);
  std::string ExtractRemainingPath(const LocationConfig* location,
                                  const HttpRequest& request) const;
  std::string CombinePaths(const std::string& root_path,
                          const std::string& remaining_path) const;
  bool IsCgiPath(const HttpRequest& request,
                const LocationConfig* location) const;
  std::string ValidateAndCheckPath(const std::string& path, bool* is_directory) const;
  void HandleGetRequest(const HttpRequest& request,
                       HttpResponse* response,
                       const ServerConfig& config);
  bool ShouldHandleAsCgi(const HttpRequest& request,
                        const std::string& path,
                        const LocationConfig* location);
  bool TryFindIndexFile(const std::string& path,
                       const LocationConfig* location,
                       std::string* index_path);
  void ServeDirectoryListing(const std::string& dir_path,
                            const std::string& request_path,
                            HttpResponse* response) const;
  void ServeRegularFile(const std::string& file_path,
                       HttpResponse* response) const;
  void HandlePostRequest(const HttpRequest& request,
                        HttpResponse* response,
                        const ServerConfig& config);
  bool IsMultipartFormData(const HttpRequest& request) const;
  void HandleMultipartUpload(const LocationConfig* location,
                           const HttpRequest& request) const;
  std::string EnsureUploadDirectory(const LocationConfig* location) const;
  bool SaveUploadedFiles(const std::vector<FileUpload>& files,
                        const std::string& upload_path) const;
  void SetSuccessResponse(HttpResponse* response,
                         const std::string& location,
                         const std::string& message) const;
  void HandleChunkedRequest(HttpResponse* response) const;
  void HandleDeleteRequest(const HttpRequest& request,
                         HttpResponse* response,
                         const ServerConfig& config);
  bool DeleteFile(const std::string& path) const;

  HttpResponse* response_;
  ServerConfig* config_;
};

class ResponseDirector {
 public:
  ResponseDirector(ResponseBuilder* builder);
  ~ResponseDirector();

  void ConstructResponse(const HttpRequest& request);
  void ConstructErrorResponse(int status_code,
                              const std::string& status_message);
  void SetResponse(HttpResponse* response);
  HttpResponse* GetResponse();
  void SetClientFd(int fd);
 private:
  ResponseBuilder* builder_;
};

#endif  // RESPONSE_BUILDER_H_
