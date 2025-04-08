#include "../../inc/Request/request_parser.h"


const std::size_t default_max_line_size = 8192;

RequestParser::RequestParser()
    : state_(START),
      content_length_(static_cast<std::size_t>(-1)),
      body_expected_(false),
      is_chunked_(false),
      config_(NULL),
      chunk_state_(CHUNK_SIZE),
      current_chunk_size_(0) {}

RequestParser::RequestParser(ServerConfig* config)
    : state_(START),
      content_length_(static_cast<std::size_t>(-1)),
      body_expected_(false),
      is_chunked_(false),
      config_(config),
      chunk_state_(CHUNK_SIZE),
      current_chunk_size_(0) {}

RequestParser::~RequestParser() {}

void RequestParser::SetConfig(ServerConfig* config) {
  config_ = config;
}

ServerConfig* RequestParser::GetServer() const {
  return config_;
}

RequestParser::ParsingStatus RequestParser::Consume(const std::string &data) {
  buffer_.append(data);

  try {
    return ProcessCurrentState();
  } catch (const std::exception &e) {
    Reset();
    throw;
  }
}

RequestParser::ParsingStatus RequestParser::ProcessCurrentState() {
  switch (state_) {
    case START:
      return ProcessStartState();

    case HEADERS:
      return ProcessHeadersState();

    case BODY:
      return ProcessBodyState();

    case COMPLETE:
      return PARSE_COMPLETE;
  }

  return PARSE_INCOMPLETE;
}

RequestParser::ParsingStatus RequestParser::ProcessStartState() {
  if (!ParseStartLine()) {
    return PARSE_INCOMPLETE;
  }

  state_ = HEADERS;
  if (buffer_.empty()) {
    return PARSE_INCOMPLETE;
  }

  return ProcessHeadersState();
}

RequestParser::ParsingStatus RequestParser::ProcessHeadersState() {
  if (!ParseHeaders()) {
    return PARSE_INCOMPLETE;
  }

  state_ = (body_expected_ || is_chunked_) ? BODY : COMPLETE;
  if (state_ == COMPLETE) {
    return PARSE_COMPLETE;
  }

  if (state_ == BODY && buffer_.empty()) {
    return PARSE_INCOMPLETE;
  }

  return ProcessBodyState();
}

RequestParser::ParsingStatus RequestParser::ProcessBodyState() {
  if (is_chunked_) {
    if (ParseChunkedBody()) {
      state_ = COMPLETE;
      return PARSE_COMPLETE;
    }
    return PARSE_INCOMPLETE;
  }

  if (ParseBody()) {
    state_ = COMPLETE;
    return PARSE_COMPLETE;
  }

  return PARSE_INCOMPLETE;
}

bool RequestParser::ParseStartLine() {
  std::string::size_type pos = buffer_.find("\r\n");
  if (pos == std::string::npos) {
    return false;
  }

  std::string line = buffer_.substr(0, pos);
  line = libft::FT_Trim(line);

  if (line.length() > default_max_line_size) {
    throw UriTooLongException();
  }

  std::string method, uri, version;
  ExtractRequestLine(line, method, uri, version);

  buffer_.erase(0, pos + 2);

  ParseUri(uri);

  request_.SetMethod(method);
  request_.SetVersion(version);

  return true;
}

void RequestParser::ExtractRequestLine(const std::string &line,
                                     std::string &method, std::string &uri,
                                     std::string &version) {
  if (line.empty()) {
    throw BadRequestException();
  }

  std::vector<std::string> tokens = SplitIntoTokens(line);
  if (tokens.size() != 3) {
    throw BadRequestException();
  }

  method = tokens[0];
  uri = tokens[1];
  version = tokens[2];

  ValidateVersion(version);
}

std::vector<std::string> RequestParser::SplitIntoTokens(const std::string &line) {
  std::istringstream stream(line);
  std::vector<std::string> tokens;
  std::string token;

  while (stream >> token) {
    tokens.push_back(libft::FT_Trim(token));
  }

  return tokens;
}

void RequestParser::ValidateVersion(const std::string &version) {
  if (version.empty()) {
    throw BadRequestException();
  }

  if (version == "HTTP/1.1") {
    return;
  }

  if (version.substr(0, 8) == "HTTP/1.1" && version.size() > 8) {
    throw BadRequestException();
  }

  if (version.substr(0, 5) == "HTTP/") {
    throw HttpVersionNotSupportedException();
  }

  throw BadRequestException();
}

void RequestParser::ParseUri(std::string &uri) {
  if (IsAbsoluteUri(uri)) {
    ExtractHostFromAbsoluteUri(uri);
  }

  uri = parsing_utils::UrlDecode(uri);
  SetPathAndQueryString(uri);
}

bool RequestParser::IsAbsoluteUri(const std::string &uri) {
  return uri.substr(0, 7) == "http://";
}

void RequestParser::ExtractHostFromAbsoluteUri(std::string &uri) {
  std::string::size_type host_start = 7;
  std::string::size_type path_start = uri.find('/', host_start);

  if (path_start == std::string::npos) {
    throw BadRequestException();
  }

  absolute_uri_host = uri.substr(host_start, path_start - host_start);
  uri = uri.substr(path_start);
}

void RequestParser::SetPathAndQueryString(const std::string &uri) {
  std::string::size_type query_pos = uri.find('?');
  if (query_pos != std::string::npos) {
    std::string path = uri.substr(0, query_pos);
    std::string query = uri.substr(query_pos + 1);

    request_.SetPath(path);
    request_.SetQueryString(query);
  } else {
    request_.SetPath(uri);
  }
}

bool RequestParser::ParseHeaders() {
  std::string::size_type pos;

  while ((pos = buffer_.find("\r\n")) != std::string::npos) {
    if (pos == 0) {
      buffer_.erase(0, 2);
      ValidateRequest();
      return true;
    }

    if (IsInvalidHeaderStart()) {
      throw BadRequestException();
    }

    std::string line = buffer_.substr(0, pos);
    buffer_.erase(0, pos + 2);

    if (IsHeaderTooLarge(line)) {
      throw BadRequestException("Request Header Or Cookie Too Large");
    }

    ProcessHeaderLine(line);
  }

  return false;
}

bool RequestParser::IsInvalidHeaderStart() {
  return buffer_[0] == ' ' || buffer_[0] == '\t';
}

bool RequestParser::IsHeaderTooLarge(const std::string &line) {
  return line.length() > default_max_line_size;
}

void RequestParser::ProcessHeaderLine(const std::string &line) {
  std::size_t colon_pos = line.find(':');
  if (colon_pos == std::string::npos) {
    throw BadRequestException();
  }

  std::string key = libft::FT_Trim(line.substr(0, colon_pos));
  std::string value = libft::FT_Trim(line.substr(colon_pos + 1));

  ValidateHeaderKey(key);


  AddHeaderToRequest(key, value);
}

void RequestParser::AddHeaderToRequest(const std::string &key, const std::string &value) {
  std::string lower_key = libft::FT_ToLower(key);

  if (IsHostHeaderDuplicate(lower_key)) {
    throw BadRequestException();
  }

  request_.SetHeader(lower_key, value);

  ProcessSpecialHeaders(lower_key, value);
}

bool RequestParser::IsHostHeaderDuplicate(const std::string &lower_key) {
  return lower_key == "host" && request_.GetHeaders().count("host") > 0;
}

void RequestParser::ProcessSpecialHeaders(const std::string &lower_key, const std::string &value) {
  if (lower_key == "content-length") {
    ProcessContentLength(value);
  } else if (lower_key == "transfer-encoding") {
    ProcessTransferEncoding(value);
  } else if (lower_key == "content-type") {
    ParseContentType(value);
  }
}

bool RequestParser::ValidateToken(char c) {
  if (std::isalnum(c)) return true;
  return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
         c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
         c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

void RequestParser::ValidateHeaderKey(const std::string &key) {
  if (key.empty()) {
    throw std::invalid_argument("Empty header key");
  }

  for (std::string::const_iterator it = key.begin(); it != key.end(); ++it) {
    if (!ValidateToken(*it)) {
      throw std::invalid_argument("Invalid header key : " + key);
    }
  }
}

void RequestParser::ProcessContentLength(const std::string &value) {
  if (is_chunked_) {
    throw BadRequestException();
  }

  std::istringstream iss(value);
  time_t length;
  if (!(iss >> length) || length < 0) {
    throw LengthRequiredException();
  }

  content_length_ = length;
  request_.SetContentLength(length);
  body_expected_ = (length > 0);
}

void RequestParser::ProcessTransferEncoding(const std::string &value) {
  if (body_expected_) {
    throw BadRequestException();
  }

  if (value != "chunked") {
    throw BadRequestException();
  }

  is_chunked_ = true;
  request_.SetIsChunked(true);
}

void RequestParser::ParseContentType(const std::string &content_type) {
  std::string::size_type semicolon_pos = content_type.find(';');
  std::string base_type = content_type.substr(0, semicolon_pos);

  request_.SetContentType(base_type);

  if (base_type == "multipart/form-data" || base_type == "multipart/mixed") {
    if (semicolon_pos == std::string::npos) {
      throw BadRequestException();
    }

    std::string params = content_type.substr(semicolon_pos + 1);
    std::string::size_type boundary_pos = params.find("boundary=");
    if (boundary_pos == std::string::npos) {
      throw BadRequestException();
    }

    std::string boundary = params.substr(boundary_pos + 9);
    if (boundary.empty()) {
      throw BadRequestException();
    }

    if (boundary[0] == '"') {
      if (boundary.length() < 2 || boundary[boundary.length() - 1] != '"') {
        throw BadRequestException();
      }
      boundary = boundary.substr(1, boundary.length() - 2);
    }

    request_.SetBoundary(boundary);
  }
}

bool RequestParser::ParseChunkedBody() {
  std::string complete_body;

  if (!request_.GetBody().empty()) {
    complete_body = request_.GetBody();
  }

  while (true) {
    switch (chunk_state_) {
      case CHUNK_SIZE:
        if (!ProcessChunkSize(complete_body)) {
          return false;
        }
        break;

      case CHUNK_DATA:
        if (!ProcessChunkData(complete_body)) {
          return false;
        }
        break;

      case CHUNK_TRAILER:
        if (!ProcessChunkTrailer(complete_body)) {
          return false;
        }
        break;

      case CHUNK_COMPLETE:
        FinalizeChunkedBody(complete_body);
        return true;
    }
  }
}

bool RequestParser::ProcessChunkSize(std::string &complete_body) {
  std::string::size_type pos = buffer_.find("\r\n");
  if (pos == std::string::npos) {
    request_.SetBody(complete_body);
    return false;
  }

  current_chunk_size_ = GetChunkSize(pos);
  buffer_.erase(0, pos + 2);

  if (current_chunk_size_ == 0) {
    chunk_state_ = CHUNK_TRAILER;
  } else {
    chunk_state_ = CHUNK_DATA;
  }

  return true;
}

bool RequestParser::ProcessChunkData(std::string &complete_body) {
  if (buffer_.length() < current_chunk_size_ + 2) {
    if (buffer_.find("\r\n") != std::string::npos) {
      ResetChunkProcessing();
      throw BadRequestException();
    }
    request_.SetBody(complete_body);
    return false;
  }

  complete_body.append(buffer_.substr(0, current_chunk_size_));
  buffer_.erase(0, current_chunk_size_);

  if (buffer_.substr(0, 2) != "\r\n") {
    ResetChunkProcessing();
    throw BadRequestException();
  }

  buffer_.erase(0, 2);
  chunk_state_ = CHUNK_SIZE;

  return true;
}

void RequestParser::ResetChunkProcessing() {
  chunk_state_ = CHUNK_SIZE;
  current_chunk_size_ = 0;
}

bool RequestParser::ProcessChunkTrailer(std::string &complete_body) {
  std::string::size_type pos = buffer_.find("\r\n");
  if (pos == std::string::npos) {
    request_.SetBody(complete_body);
    return false;
  }

  buffer_.erase(0, pos + 2);
  chunk_state_ = CHUNK_COMPLETE;

  return true;
}

void RequestParser::FinalizeChunkedBody(const std::string &complete_body) {
  request_.SetContentLength(complete_body.length());
  request_.SetBody(complete_body);
  ResetChunkProcessing();
}

unsigned int RequestParser::GetChunkSize(std::string::size_type pos) {
  std::string chunk_size_str = buffer_.substr(0, pos);
  std::string::size_type semicolon_pos = chunk_size_str.find(';');
  if (semicolon_pos != std::string::npos) {
    chunk_size_str = chunk_size_str.substr(0, semicolon_pos);
  }

  unsigned int chunk_size;
  std::istringstream iss(chunk_size_str);
  if (!(iss >> std::hex >> chunk_size)) {
    throw BadRequestException();
  }
  return chunk_size;
}

void RequestParser::ValidateRequest() {
  ValidateHost();
  ValidatePostMethod();
}

void RequestParser::ValidateHost() {
  if (!absolute_uri_host.empty()) {
    request_.SetHeader("host", absolute_uri_host);
  } else if (request_.GetHeaders().find("host") == request_.GetHeaders().end()) {
    throw BadRequestException();
  }
}

void RequestParser::ValidatePostMethod() {
  if (request_.GetMethod() == "POST") {
    if (content_length_ == static_cast<std::size_t>(-1) && !is_chunked_) {
      throw BadRequestException();
    }
  }
}

bool RequestParser::ParseBody() {
  if (content_length_ == static_cast<std::size_t>(-1)) {
    return true;
  }

  std::string current_body = request_.GetBody();
  std::size_t received_so_far = current_body.length();
  std::size_t remaining = content_length_ - received_so_far;

  CheckBodySize(received_so_far);

  if (buffer_.length() >= remaining) {
    return ProcessCompleteBody(current_body, remaining);
  } else {
    return ProcessPartialBody(current_body);
  }
}

void RequestParser::CheckBodySize(std::size_t received_so_far) {
  if (config_) {
    const LocationConfig* location = FindMatchingLocation(request_, config_);
    if (location && received_so_far + buffer_.length() > location->GetClientMaxBodySize()) {
      throw ContentTooLargeException();
    }
  }
}

bool RequestParser::ProcessCompleteBody(const std::string &current_body, std::size_t remaining) {
  std::string new_part = buffer_.substr(0, remaining);
  buffer_.erase(0, remaining);

  std::string complete_body = current_body + new_part;

  if (request_.IsMultipart()) {
    ParseMultipartBody(complete_body);
  } else {
    request_.SetBody(complete_body);
  }

  return true;
}

bool RequestParser::ProcessPartialBody(const std::string &current_body) {
  std::string new_part = buffer_;
  buffer_.clear();

  request_.SetBody(current_body + new_part);

  return false;
}

void RequestParser::ParseMultipartBody(const std::string &body) {
  MultipartData multipart_data;
  std::string boundary = "--" + request_.GetBoundary();

  std::string::size_type pos = 0;
  std::string::size_type next_pos;

  while ((next_pos = body.find(boundary, pos)) != std::string::npos) {
    if (pos > 0) {
      ProcessMultipartPart(body.substr(pos, next_pos - pos - 2), multipart_data);
    }
    pos = next_pos + boundary.length() + 2;
  }

  request_.SetMultipartData(multipart_data);
  request_.SetBody(body);
}

void RequestParser::ProcessMultipartPart(const std::string &part,
                                     MultipartData &multipart_data) {
  std::string::size_type header_end = part.find("\r\n\r\n");
  if (header_end == std::string::npos) {
    throw std::invalid_argument("Invalid multipart format");
  }

  std::string headers = part.substr(0, header_end);
  std::string content = part.substr(header_end + 4);

  ParsePartHeaders(headers, multipart_data, content);
}

void RequestParser::ParsePartHeaders(const std::string &headers,
                                   MultipartData &multipart_data,
                                   const std::string &content) {
  PartInfo part_info = ExtractPartInfo(headers);
  StorePartContent(part_info, multipart_data, content);
}

RequestParser::PartInfo RequestParser::ExtractPartInfo(const std::string &headers) {
  PartInfo info;

  std::istringstream header_stream(headers);
  std::string header_line;
  while (std::getline(header_stream, header_line)) {
    if (header_line.empty() || header_line == "\r") continue;

    if (header_line.find("Content-Disposition: ") == 0) {
      std::string disposition = header_line.substr(21);
      ParseContentDisposition(disposition, info.field_name, info.filename);
    } else if (header_line.find("Content-Type: ") == 0) {
      info.content_type = header_line.substr(14);
    }
  }

  return info;
}

void RequestParser::StorePartContent(const PartInfo &info,
                                   MultipartData &multipart_data,
                                   const std::string &content) {
  if (!info.filename.empty()) {
    multipart_data.AddFile(info.field_name, info.filename, content, info.content_type);
  } else {
    multipart_data.AddField(info.field_name, content);
  }
}

void RequestParser::ParseContentDisposition(const std::string &disposition,
                                          std::string &field_name,
                                          std::string &filename) {
  ExtractFieldName(disposition, field_name);
  ExtractFilename(disposition, filename);
}

void RequestParser::ExtractFieldName(const std::string &disposition, std::string &field_name) {
  std::string::size_type name_pos = disposition.find("name=\"");
  if (name_pos != std::string::npos) {
    name_pos += 6;
    std::string::size_type name_end = disposition.find("\"", name_pos);
    if (name_end != std::string::npos) {
      field_name = disposition.substr(name_pos, name_end - name_pos);
    }
  }
}

void RequestParser::ExtractFilename(const std::string &disposition, std::string &filename) {
  std::string::size_type filename_pos = disposition.find("filename=\"");
  if (filename_pos != std::string::npos) {
    filename_pos += 10;
    std::string::size_type filename_end = disposition.find("\"", filename_pos);
    if (filename_end != std::string::npos) {
      filename = disposition.substr(filename_pos, filename_end - filename_pos);
    }
  }
}

void RequestParser::Reset() {
  state_ = START;
  buffer_.clear();
  content_length_ = static_cast<std::size_t>(-1);
  body_expected_ = false;
  is_chunked_ = false;
  request_ = HttpRequest();
  chunk_state_ = CHUNK_SIZE;
  current_chunk_size_ = 0;
}

const HttpRequest &RequestParser::GetRequest() const {
  if (state_ != COMPLETE) {
    throw std::runtime_error("Request is not complete");
  }
  return request_;
}


