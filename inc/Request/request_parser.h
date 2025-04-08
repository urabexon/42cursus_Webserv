#ifndef WEBSERV_inc_REQUEST_PARSER_H_
#define WEBSERV_inc_REQUEST_PARSER_H_

#include "../Exception/http_exception.h"
#include "../Config/server_config.h"
#include "../Util/config_utils.h"
#include "../Util/parsing_utils.h"
#include "http_request.h"

class RequestParser {
 public:
  enum ParsingStatus { PARSE_INCOMPLETE, PARSE_COMPLETE };

  RequestParser();
  RequestParser(ServerConfig* config);
  ~RequestParser();

  ParsingStatus Consume(const std::string& data);
  const HttpRequest& GetRequest() const;
  void Reset();
  void SetConfig(ServerConfig* config);
  ServerConfig* GetServer() const;

 private:
  typedef std::map<std::string, std::string> Headers;

  struct PartInfo {
    std::string field_name;
    std::string filename;
    std::string content_type;
  };

  enum State { START, HEADERS, BODY, COMPLETE };
  enum ChunkState { CHUNK_SIZE, CHUNK_DATA, CHUNK_TRAILER, CHUNK_COMPLETE };

  ParsingStatus ProcessCurrentState();
  ParsingStatus ProcessStartState();
  ParsingStatus ProcessHeadersState();
  ParsingStatus ProcessBodyState();

  bool ParseStartLine();
  bool ParseHeaders();
  bool ParseBody();
  bool IsInvalidHeaderStart();
  bool IsHeaderTooLarge(const std::string &line);
  bool ValidateToken(char c);

  std::vector<std::string> SplitIntoTokens(const std::string &line);
  void ValidateVersion(const std::string &version);
  bool IsAbsoluteUri(const std::string &uri);
  void ExtractHostFromAbsoluteUri(std::string &uri);
  void SetPathAndQueryString(const std::string &uri);

  void ProcessHeaderLine(const std::string &line);
  void AddHeaderToRequest(const std::string &key, const std::string &value);
  bool IsHostHeaderDuplicate(const std::string &lower_key);
  void ProcessSpecialHeaders(const std::string &lower_key, const std::string &value);

  void CheckBodySize(std::size_t received_so_far);
  bool ProcessCompleteBody(const std::string &current_body, std::size_t remaining);
  bool ProcessPartialBody(const std::string &current_body);

  bool ProcessChunkSize(std::string &complete_body);
  bool ProcessChunkData(std::string &complete_body);
  bool ProcessChunkTrailer(std::string &complete_body);
  void FinalizeChunkedBody(const std::string &complete_body);
  void ResetChunkProcessing();

  void ValidateRequest();
  void ValidateHost();
  void ValidatePostMethod();
  void ParseUri(std::string& uri);
  void ValidateHeaderKey(const std::string& key);
  void ProcessContentLength(const std::string& value);
  void ProcessTransferEncoding(const std::string& value);
  void ParseContentType(const std::string& content_type);
  void ParseMultipartBody(const std::string& body);
  void ProcessMultipartPart(const std::string& part,
                          MultipartData& multipart_data);
  void ParsePartHeaders(const std::string &headers,
                      MultipartData &multipart_data,
                      const std::string &content);
  PartInfo ExtractPartInfo(const std::string &headers);
  void StorePartContent(const PartInfo &info,
                      MultipartData &multipart_data,
                      const std::string &content);
  void ParseContentDisposition(const std::string& disposition,
                             std::string& field_name, std::string& filename);
  void ExtractFieldName(const std::string &disposition, std::string &field_name);
  void ExtractFilename(const std::string &disposition, std::string &filename);
  bool ParseChunkedBody();
  void ExtractRequestLine(const std::string& line, std::string& method,
                        std::string& uri, std::string& version);
  unsigned int GetChunkSize(std::string::size_type pos);

  State state_;
  std::string buffer_;
  HttpRequest request_;
  std::size_t content_length_;
  bool body_expected_;
  bool is_chunked_;
  std::string absolute_uri_host;
  ServerConfig* config_;
  ChunkState chunk_state_;
  unsigned int current_chunk_size_;
};

#endif  // WEBSERV_inc_REQUEST_PARSER_H_
