#include "../../inc/Config/config_parser.h"

ConfigParser::ConfigParser() : _current_line("") {}

ConfigParser::~ConfigParser() {}

HttpConfig *ConfigParser::Parse(const std::string &filename)
{
  HttpConfig* http_config = new HttpConfig();

  try
  {
    OpenConfigFile(filename);
    FindAndParseHttpBlock(http_config);
    CheckAndCreateDefaultServer(http_config);
  }
  catch(const std::exception& e)
  {
    _input.close();
    delete http_config;
    throw;
  }

  _input.close();
  return http_config;
}

void ConfigParser::OpenConfigFile(const std::string &filename)
{
  _input.open(filename.c_str());
  if (!_input.is_open())
  {
    throw std::runtime_error("Failed to open configuration file");
  }
}

void ConfigParser::FindAndParseHttpBlock(HttpConfig* http_config)
{
  bool http_declared = false;

  while (std::getline(_input, _current_line))
  {
    _current_line = libft::FT_Trim(_current_line);
    if (_current_line.empty() || _current_line[0] == '#')
    {
      _current_line.clear();
      continue;
    }

    if (!http_declared && _current_line.substr(0, 4) == "http")
    {
      _current_line = libft::FT_Trim(_current_line.substr(4));
      http_declared = true;
    }

    if (http_declared && _current_line[0] == '{')
    {
      _current_line = libft::FT_Trim(_current_line.substr(1));
      ParseHttpBlock(http_config);
      return;
    }
    else if (!http_declared)
    {
      throw std::runtime_error("No HTTP block in configuration");
    }
  }

  throw std::runtime_error("HTTP block not found or incomplete");
}

void ConfigParser::CheckAndCreateDefaultServer(HttpConfig* http_config)
{
  if (http_config->GetServers().empty())
  {
    CreateDefaultServerIfNeeded(http_config);
  }
}

void ConfigParser::ReadNextNonEmptyLine()
{
  std::string line;
  if (!std::getline(_input, line))
  {
    throw std::runtime_error("unexpected end of file");
  }
  line = libft::FT_Trim(parsing_utils::RemoveComments(line));
  if (!line.empty())
  {
    _current_line = line;
  }
  else
  {
    _current_line.clear();
  }
}

bool ConfigParser::IsBlockEnd()
{
  if (_current_line[0] == '}')
  {
    _current_line = libft::FT_Trim(_current_line.substr(1));
    return true;
  }
  return false;
}

void ConfigParser::ParseHttpBlock(HttpConfig *http_config)
{
  while (true)
  {
    if (_current_line.empty())
    {
      try
      {
        ReadNextNonEmptyLine();
        continue;
      }
      catch (const std::exception &)
      {
        throw std::runtime_error("unexpected end of file, expecting \"}\"");
      }
    }

    if (IsBlockEnd())
    {
      CreateDefaultServerIfNeeded(http_config);
      return;
    }

    if (_current_line.substr(0, 6) == "server")
    {
      ParseServerBlockEntry(http_config);
    }
    else
    {
      HandleHttpDirectiveLine(http_config);
    }
  }
}

void ConfigParser::CreateDefaultServerIfNeeded(HttpConfig *http_config)
{
  if (http_config->GetServers().empty())
  {
    ServerConfig *default_server = new ServerConfig(http_config);
    default_server->AddListenDirective("0.0.0.0", 8000);
    default_server->SetRoot("./var");
    default_server->SetDefault(true);
    http_config->AddServer(default_server);
  }
}

void ConfigParser::ParseServerBlockEntry(HttpConfig *http_config)
{
  ServerConfig *server_config = new ServerConfig(http_config);
  _current_line = libft::FT_Trim(_current_line.substr(6));

  if (_current_line[0] != '{')
  {
    delete server_config;
    throw std::runtime_error("expected \"{\" after \"server\"");
  }

  _current_line = libft::FT_Trim(_current_line.substr(1));

  try
  {
    ParseServerBlock(server_config);
    http_config->AddServer(server_config);
  }
  catch (const std::exception &)
  {
    delete server_config;
    throw;
  }
}

void ConfigParser::HandleHttpDirectiveLine(HttpConfig *http_config)
{
  std::string original_line = _current_line;
  JoinMultiLineDirective();
  HandleHttpDirective(http_config);

  if (_current_line == original_line)
  {
    _current_line.clear();
  }
}

void ConfigParser::JoinMultiLineDirective()
{
  while (_current_line.find(';') == std::string::npos)
  {
    std::string next_line;
    if (!std::getline(_input, next_line))
    {
      throw std::runtime_error("directive is not terminated by \";\"");
    }
    next_line = libft::FT_Trim(parsing_utils::RemoveComments(next_line));
    if (!next_line.empty())
    {
      _current_line += " " + next_line;
    }
  }
}

void ConfigParser::ParseServerBlock(ServerConfig *server_config)
{
  while (true)
  {
    if (_current_line.empty())
    {
      if (!std::getline(_input, _current_line))
      {
        throw std::runtime_error("unexpected end of file, expecting \"}\"");
      }
      _current_line = libft::FT_Trim(_current_line);
      if (_current_line.empty() || _current_line[0] == '#')
      {
        _current_line.clear();
        continue;
      }
    }

    _current_line = parsing_utils::RemoveComments(_current_line);
    _current_line = libft::FT_Trim(_current_line);

    if (IsBlockEnd())
    {
      SetServerDefaults(server_config);
      return;
    }

    if (_current_line.substr(0, 8) == "location")
    {
      ParseLocationBlockEntry(server_config);
    }
    else
    {
      HandleServerDirective(server_config);
    }
  }
}

void ConfigParser::SetServerDefaults(ServerConfig *server_config)
{
  if (server_config->GetKeepaliveTimeout() == -1)
  {
    server_config->SetKeepaliveTimeout(75 * 1000);
  }

  if (server_config->GetListenDirectives().empty())
  {
    server_config->AddListenDirective("*", 8080);
  }
}

void ConfigParser::ParseLocationBlockEntry(ServerConfig *server_config)
{
  LocationConfig *location_config = new LocationConfig(server_config);
  try
  {
    _current_line = libft::FT_Trim(_current_line.substr(8));
    ParseLocationBlock(location_config);
    server_config->AddLocation(location_config);
  }
  catch (const std::exception &)
  {
    delete location_config;
    throw;
  }
}

void ConfigParser::ParseLocationBlock(LocationConfig *location_config)
{
  SetupLocationPath(location_config);
  ParseLocationDirectives(location_config);
  SetLocationDefaults(location_config);
}

void ConfigParser::SetupLocationPath(LocationConfig *location_config)
{
  while (true)
  {
    if (_current_line.empty())
    {
      if (!ReadNextLocationLine())
      {
        delete location_config;
        throw std::runtime_error("Unexpected end of file in location block");
      }

      if (_current_line.empty())
        continue;
    }

    std::string path = ParseLocationPath();
    location_config->SetPath(path);
    break;
  }
}

bool ConfigParser::ReadNextLocationLine()
{
  if (!std::getline(_input, _current_line))
    return false;

  _current_line = libft::FT_Trim(_current_line);

  if (_current_line.empty() || _current_line[0] == '#')
  {
    _current_line.clear();
    return true;
  }

  return true;
}

void ConfigParser::ParseLocationDirectives(LocationConfig *location_config)
{
  while (true)
  {
    if (_current_line.empty())
    {
      if (!ReadNextLocationLine())
      {
        delete location_config;
        throw std::runtime_error("Unexpected end of file in location block");
      }

      if (_current_line.empty())
        continue;
    }

    _current_line = parsing_utils::RemoveComments(_current_line);
    _current_line = libft::FT_Trim(_current_line);

    if (IsBlockEnd())
      return;

    HandleLocationDirective(location_config);
  }
}

void ConfigParser::SetLocationDefaults(LocationConfig *location_config)
{
  if (location_config->GetIndexFiles().empty())
  {
    location_config->ClearIndexFiles();
    location_config->AddIndexFile("index.html");
  }
}

void ConfigParser::HandleHttpDirective(HttpConfig *http_config)
{
  if (_current_line.empty() || _current_line[0] == '#')
    return;

  Directive directive = ExtractDirective();

  if (directive.first.empty() || directive.first[0] == '#')
    return;

  if (directive.first == "keepalive_timeout")
    ParseKeepaliveTimeoutDirective(directive.second, http_config);
  else
    HandleCommonDirective(directive, http_config);
}

void ConfigParser::HandleServerDirective(ServerConfig *server_config)
{
  Directive directive = ExtractDirective();

  if (directive.first == "listen")
    ParseListenDirective(directive.second, server_config);
  else if (directive.first == "server_name")
    ParseServerNameDirective(directive.second, server_config);
  else if (directive.first == "return")
    ParseServerRedirectDirective(directive.second, server_config);
  else if (directive.first == "keepalive_timeout")
    ParseKeepaliveTimeoutDirective(directive.second, server_config);
  else
    HandleCommonDirective(directive, server_config);
}

void ConfigParser::HandleLocationDirective(LocationConfig *location_config)
{
  Directive directive = ExtractDirective();

  if (directive.first == "accept_methods")
    ParseLimitExceptDirective(directive.second, location_config);
  else if (directive.first == "return")
    ParseLocationRedirectDirective(directive.second, location_config);
  else if (directive.first == "cgi_pass")
    ParseCgiPassDirective(directive.second, location_config);
  else if (directive.first == "cgi_read_timeout")
    ParseCgiReadTimeoutDirective(directive.second, location_config);
  else if (directive.first == "upload_path")
  {
    std::string path = parsing_utils::GetNextToken(directive.second);
    location_config->SetUploadPath(path);
  }
  else if (directive.first == "keepalive_timeout")
    ParseKeepaliveTimeoutDirective(directive.second, location_config);
  else
    HandleCommonDirective(directive, location_config);
}

void ConfigParser::HandleCommonDirective(const Directive &directive, BaseConfig *config)
{
  if (directive.first == "root")
    config->SetRoot(directive.second);
  else if (directive.first == "client_max_body_size")
    ParseClientMaxBodySizeDirective(directive.second, config);
  else if (directive.first == "error_page")
    ParseErrorPageDirective(directive.second, config);
  else if (directive.first == "autoindex")
    ParseAutoIndexDirective(directive.second, config);
  else if (directive.first == "index")
    ParseIndexDirective(directive.second, config);
  else if (directive.first[0] != '#')
    throw std::runtime_error("unknown directive: " + directive.first);
}

void ConfigParser::ParseClientMaxBodySizeDirective(const std::string &value, BaseConfig *config)
{
  if (value.empty())
    throw std::runtime_error("client_max_body_size value is empty");

  std::string number_str = value;
  off_t multiplier = 1;

  ExtractSizeAndMultiplier(value, number_str, multiplier);
  ValidateNumericValue(number_str);

  off_t size = ParseSizeValue(number_str, multiplier);
  config->SetClientMaxBodySize(size);
}

void ConfigParser::ExtractSizeAndMultiplier(const std::string &value, std::string &number_str, off_t &multiplier)
{
  char unit = value[value.length() - 1];

  if (unit == 'k' || unit == 'K' || unit == 'm' || unit == 'M' || unit == 'g' ||
      unit == 'G')
  {
    number_str = value.substr(0, value.length() - 1);

    if (unit == 'k' || unit == 'K')
      multiplier = 1024;
    else if (unit == 'm' || unit == 'M')
      multiplier = 1024 * 1024;
    else if (unit == 'g' || unit == 'G')
      multiplier = 1024 * 1024 * 1024;
  }
}

void ConfigParser::ValidateNumericValue(const std::string &number_str)
{
  for (std::string::const_iterator it = number_str.begin();
       it != number_str.end(); ++it)
  {
    if (!std::isdigit(*it))
      throw std::runtime_error("client_max_body_size contains non-numeric characters");
  }
}

off_t ConfigParser::ParseSizeValue(const std::string &number_str, off_t multiplier)
{
  std::istringstream iss(number_str);
  off_t size;

  if (!(iss >> size) || size < 0)
    throw std::runtime_error("Invalid client_max_body_size value");

  if (size > (std::numeric_limits<off_t>::max() / multiplier))
    throw std::runtime_error("client_max_body_size value too large");

  return size * multiplier;
}

void ConfigParser::ParseErrorPageDirective(const std::string &value, BaseConfig *config)
{
  std::vector<std::string> tokens;
  std::stringstream ss(value);
  std::string token;

  while (ss >> token)
    tokens.push_back(token);

  if (tokens.size() < 2)
    throw std::runtime_error("Invalid error_page directive format");

  std::string page = tokens.back();
  tokens.pop_back();

  for (std::vector<std::string>::const_iterator it = tokens.begin();
       it != tokens.end(); ++it)
  {
    std::stringstream code_ss(*it);
    int code;
    code_ss >> code;

    if (code_ss.fail() || code < 300 || code > 599)
      throw std::runtime_error("Invalid error code in error_page directive");
    config->AddErrorPage(code, page);
  }
}

void ConfigParser::ParseAutoIndexDirective(const std::string &value, BaseConfig *config)
{
  if (config->IsAutoindexSet())
  {
    throw std::runtime_error("\"autoindex\" directive is duplicate");
  }
  if (value != "on" && value != "off")
    throw std::runtime_error("invalid value \"" + value + "\" in \"autoindex\" directive, it must be \"on\" or \"off\"");
  config->SetAutoindex(value == "on");
}

void ConfigParser::ParseIndexDirective(const std::string &value, BaseConfig *config)
{
  if (value.empty())
  {
    throw std::runtime_error("invalid number of arguments in \"index\" directive");
  }

  std::string remaining = value;
  std::string index_file;
  bool added = false;

  while (!(index_file = parsing_utils::GetNextToken(remaining)).empty())
  {
    if (config->GetIndexFiles().size() == 0)
    {
      config->AddIndexFile(index_file);
      added = true;
      continue;
    }

    std::vector<std::string>::const_iterator it = std::find(
        config->GetIndexFiles().begin(),
        config->GetIndexFiles().end(),
        index_file);

    if (it == config->GetIndexFiles().end())
    {
      config->AddIndexFile(index_file);
      added = true;
    }
  }

  if (!added)
    throw std::runtime_error("invalid number of arguments in \"index\" directive");
}

void ConfigParser::ParseKeepaliveTimeoutDirective(const std::string &value,
                                                  BaseConfig *config)
{
  ServerConfig *server = dynamic_cast<ServerConfig *>(config);
  HttpConfig *http = dynamic_cast<HttpConfig *>(config);
  LocationConfig *location = dynamic_cast<LocationConfig *>(config);

  if ((server && server->IsKeepaliveTimeoutSet()) ||
      (http && http->IsKeepaliveTimeoutSet()) ||
      (location && location->IsKeepaliveTimeoutSet()))
  {
    throw std::runtime_error("\"keepalive_timeout\" directive is duplicate");
  }

  std::string value_copy = value;
  std::string timeout_str = parsing_utils::GetNextToken(value_copy);
  if (timeout_str.empty())
  {
    throw std::runtime_error("invalid keepalive_timeout value");
  }

  time_t timeout = ParseTimeout(timeout_str);
  if (server)
    server->SetKeepaliveTimeout(timeout);
  else if (http)
    http->SetKeepaliveTimeout(timeout);
  else if (location)
    location->SetKeepaliveTimeout(timeout);

  if (!value_copy.empty())
  {
    throw std::runtime_error("invalid keepalive_timeout value");
  }
}

void ConfigParser::ParseServerRedirectDirective(const std::string &value,
                                                BaseConfig *config)
{
  ServerConfig *server = dynamic_cast<ServerConfig *>(config);
  if (!server)
    return;

  if (!server->GetRedirect().first.empty() && server->GetRedirect().second != -1)
    return;

  const Redirection &redirect = ParseRedirectValue(value);
  server->SetRedirect(redirect.first, redirect.second);
}

void ConfigParser::ParseListenDirective(const std::string &value,
                                        ServerConfig *server)
{
  std::string host = "0.0.0.0";
  int port = 80;
  bool is_default = false;

  ParseListenAddressPart(value, host, port, is_default);

  validateDuplicatedAddress(host, port, server);
  server->AddListenDirective(host, port);
  if (is_default)
  {
    server->SetDefault(true);
  }
}

void ConfigParser::ParseListenAddressPart(const std::string &value,
                                         std::string &host,
                                         int &port,
                                         bool &is_default)
{
  std::string remaining = value;
  std::string addr_part = parsing_utils::GetNextToken(remaining);
  addr_part = parsing_utils::ExtractQuotedString(addr_part);

  ParseHostAndPort(addr_part, host, port);
  ParseListenOptions(remaining, is_default);
}

void ConfigParser::ParseHostAndPort(const std::string &addr_part,
                                   std::string &host,
                                   int &port)
{
  std::string::size_type colon = addr_part.find(':');
  if (colon != std::string::npos)
  {
    std::string host_part = addr_part.substr(0, colon);
    if (host_part.empty())
    {
      throw std::runtime_error("no host in \"" + addr_part + "\" of the \"listen\" directive");
    }
    host = ResolveHost(host_part);
    port = ParsePort(addr_part);
  }
  else
  {
    if (IsDigitsOnly(addr_part))
    {
      port = ParsePort(addr_part);
    }
    else
    {
      host = ResolveHost(addr_part);
    }
  }
}

bool ConfigParser::IsDigitsOnly(const std::string &str)
{
  for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
  {
    if (!std::isdigit(*it))
    {
      return false;
    }
  }
  return true;
}

void ConfigParser::ParseListenOptions(std::string &remaining, bool &is_default)
{
  while (!remaining.empty())
  {
    std::string token = parsing_utils::GetNextToken(remaining);

    if (token != "default_server")
    {
      throw std::runtime_error("invalid parameter \"" + token + "\"");
    }
    is_default = true;
  }
}

void ConfigParser::ParseServerNameDirective(const std::string &value,
                                            ServerConfig *server)
{
  std::string remaining = value;
  std::string token;

  while (!(token = parsing_utils::GetNextToken(remaining)).empty()) {
    std::string name = libft::FT_Trim(token);

    if (name.length() >= 2)
    {
      if ((name[0] == '"' && name[name.length() - 1] == '"') ||
          (name[0] == '\'' && name[name.length() - 1] == '\''))
      {
        name = name.substr(1, name.length() - 2);
      }
    }

    if (name.find("..") != std::string::npos)
    {
      throw std::runtime_error("invalid server name or wildcard \"" + name + "\"");
    }

    server->AddServerName(name);
  }
}

void ConfigParser::ParseLimitExceptDirective(const std::string &value,
                                             LocationConfig *location)
{
  location->ClearAcceptedMethods();

  if (value == "ALL")
  {
    location->AddAcceptedMethod("GET");
    location->AddAcceptedMethod("POST");
    location->AddAcceptedMethod("DELETE");
    return;
  }

  std::istringstream iss(value);
  std::string method;
  bool added = false;

  while (iss >> method)
  {
    if (method != "GET" && method != "POST" && method != "DELETE")
    {
      throw std::runtime_error("invalid method in limit_except: " + method);
    }
    location->AddAcceptedMethod(method);
    added = true;
  }

  if ((iss.fail() && !iss.eof()) || !added)
  {
    throw std::runtime_error("Failed to parse limit_except directive");
  }
}

Directive ConfigParser::ExtractDirective()
{
  if (_current_line.empty())
  {
    return Directive("", "");
  }

  ExtractUntilSemicolon();
  return SeparateDirectiveAndValue();
}

void ConfigParser::ExtractUntilSemicolon()
{
  size_t semicolon_pos = _current_line.find(';');
  if (semicolon_pos == std::string::npos)
  {
    throw std::runtime_error("directive is not terminated by \";\"");
  }

  size_t comment_pos = _current_line.find('#', semicolon_pos + 1);
  if (comment_pos != std::string::npos)
  {
    _current_line = _current_line.substr(0, comment_pos);
  }

  _directive_line = _current_line.substr(0, semicolon_pos);
  _current_line = libft::FT_Trim(_current_line.substr(semicolon_pos + 1));
}

Directive ConfigParser::SeparateDirectiveAndValue()
{
  size_t pos = FindFirstSpaceOutsideQuotes();

  if (pos == std::string::npos)
  {
    return std::make_pair(libft::FT_Trim(_directive_line), "");
  }
  else
  {
    std::string directive = libft::FT_Trim(_directive_line.substr(0, pos));
    std::string remaining = libft::FT_Trim(_directive_line.substr(pos + 1));
    return std::make_pair(directive, remaining);
  }
}

size_t ConfigParser::FindFirstSpaceOutsideQuotes()
{
  bool in_quotes = false;
  size_t pos = std::string::npos;

  for (size_t i = 0; i < _directive_line.length(); ++i)
  {
    if (_directive_line[i] == '"')
    {
      in_quotes = !in_quotes;
    }
    else if (!in_quotes && (_directive_line[i] == ' ' || _directive_line[i] == '\t'))
    {
      pos = i;
      break;
    }
  }

  return pos;
}

Redirection ConfigParser::ParseRedirectValue(const std::string &value)
{
  std::string remaining = value;
  std::string code_str = parsing_utils::GetNextToken(remaining);

  if (code_str.empty())
  {
    throw std::runtime_error("Invalid number of arguments in \"return\" directive");
  }

  for (std::string::const_iterator it = code_str.begin(); it != code_str.end(); ++it)
  {
    if (!std::isdigit(*it))
    {
      throw std::runtime_error("invalid return code '" + code_str + "'");
    }
  }

  int code = std::atoi(code_str.c_str());
  if (code < 0 || code > 999)
  {
    throw std::runtime_error("Return code must be between 000 and 999");
  }

  std::string next_token = parsing_utils::GetNextToken(remaining);
  std::string url = parsing_utils::ExtractQuotedString(next_token);

  if (!remaining.empty())
  {
    throw std::runtime_error("Invalid number of arguments in \"return\" directive");
  }

  return Redirection(url, code);
}

std::string ConfigParser::ResolveHost(const std::string &host_part)
{
  if (host_part == "*")
    return "0.0.0.0";

  if (IsIPv4Address(host_part))
  {
    return ValidateIPv4Address(host_part);
  }

  return ResolveHostname(host_part);
}

bool ConfigParser::IsIPv4Address(const std::string &host_part)
{
  return host_part.find_first_not_of("0123456789.") == std::string::npos;
}

std::string ConfigParser::ValidateIPv4Address(const std::string &host_part)
{
  std::istringstream iss(host_part);
  std::string octet;
  int count = 0;
  bool valid = true;

  while (std::getline(iss, octet, '.'))
  {
    count++;
    if (octet.empty() || octet.length() > 3)
    {
      valid = false;
      break;
    }
    int value = std::atoi(octet.c_str());
    if (value < 0 || value > 255)
    {
      valid = false;
      break;
    }
  }

  if (!valid || count != 4)
  {
    throw std::runtime_error("host not found in \"" + host_part + "\" of the \"listen\" directive");
  }

  return host_part;
}

std::string ConfigParser::ResolveHostname(const std::string &host_part)
{
  struct addrinfo hints, *result;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(host_part.c_str(), NULL, &hints, &result);
  if (status != 0)
  {
    throw std::runtime_error("host not found in \"" + host_part + "\" of the \"listen\" directive");
  }

  struct sockaddr_in *addr =
      reinterpret_cast<struct sockaddr_in *>(result->ai_addr);
  std::string ip = libft::FT_Ipv4ToString(addr->sin_addr);

  freeaddrinfo(result);
  return ip;
}

int ConfigParser::ParsePort(const std::string &port_str)
{
  std::string port_part = ExtractPortPart(port_str);
  ValidatePortString(port_part, port_str);
  return ConvertToPortNumber(port_part, port_str);
}

std::string ConfigParser::ExtractPortPart(const std::string &port_str)
{
  std::string port_part;
  std::string::size_type colon_pos = port_str.find(':');

  if (colon_pos != std::string::npos)
  {
    port_part = port_str.substr(colon_pos + 1);
  }
  else
  {
    port_part = port_str;
  }

  if (port_part.empty())
  {
    throw std::runtime_error("invalid number of arguments in \"listen\" directive");
  }

  return port_part;
}

void ConfigParser::ValidatePortString(const std::string &port_part, const std::string &port_str)
{
  for (std::string::const_iterator it = port_part.begin(); it != port_part.end(); ++it)
  {
    if (!std::isdigit(*it))
    {
      throw std::runtime_error("invalid port in \"" + port_str + "\" of the \"listen\" directive");
    }
  }
}

int ConfigParser::ConvertToPortNumber(const std::string &port_part, const std::string &port_str)
{
  int port;
  std::istringstream iss(port_part);
  if (!(iss >> port))
  {
    throw std::runtime_error("invalid port in \"" + port_str + "\" of the \"listen\" directive");
  }

  if (port <= 0 || port > 65535)
  {
    throw std::runtime_error("invalid port in \"" + port_str + "\" of the \"listen\" directive");
  }

  return port;
}

void ConfigParser::validateDuplicatedAddress(const std::string &host, int port, ServerConfig *server)
{
  std::ostringstream oss;
  oss << port;

  const std::vector<ListenDirective> &directives = server->GetListenDirectives();
  for (std::vector<ListenDirective>::const_iterator it = directives.begin();
       it != directives.end(); ++it)
  {
    if (it->host == host && it->port == port)
    {
      throw std::runtime_error("duplicate listen " + host + ":" + oss.str());
    }
  }
}

time_t ConfigParser::ParseTimeout(const std::string &timeout_str)
{
  std::string remaining = timeout_str;
  time_t total_ms = 0;
  std::string last_unit = "";

  while (!remaining.empty())
  {
    time_t value;
    std::string unit;

    ExtractValueAndUnit(remaining, value, unit);
    ValidateUnitOrder(last_unit, unit);

    time_t ms_value = ConvertToMilliseconds(value, unit);
    ValidateOverflow(total_ms, ms_value);

    total_ms += ms_value;
    last_unit = unit;
  }

  return total_ms;
}

void ConfigParser::ExtractValueAndUnit(std::string& remaining, time_t& value, std::string& unit)
{
  std::string num_str = ExtractNumericPart(remaining);
  size_t consumed_chars = num_str.length();

  value = ParseTimeValue(num_str);
  consumed_chars += ExtractTimeUnit(remaining, consumed_chars, unit);

  UpdateRemainingString(remaining, consumed_chars);
}

std::string ConfigParser::ExtractNumericPart(const std::string& remaining)
{
  std::string num_str;
  size_t i = 0;

  while (i < remaining.length() && std::isdigit(remaining[i]))
  {
    num_str += remaining[i];
    i++;
  }

  if (num_str.empty())
  {
    throw std::runtime_error("\"keepalive_timeout\" directive invalid value");
  }

  return num_str;
}

time_t ConfigParser::ParseTimeValue(const std::string& num_str)
{
  time_t value;
  std::istringstream iss(num_str);
  if (!(iss >> value) || value < 0)
  {
    throw std::runtime_error("\"keepalive_timeout\" directive invalid value");
  }
  return value;
}

size_t ConfigParser::ExtractTimeUnit(const std::string& remaining, size_t start_pos, std::string& unit)
{
  if (start_pos < remaining.length())
  {
    if (remaining[start_pos] == 'm' && start_pos + 1 < remaining.length() && remaining[start_pos + 1] == 's')
    {
      unit = "ms";
      return 2;
    }
    else if (remaining[start_pos] == 'd' || remaining[start_pos] == 'h' ||
              remaining[start_pos] == 'm' || remaining[start_pos] == 's')
    {
      unit = std::string(1, remaining[start_pos]);
      return 1;
    }
  }
  else
  {
    unit = "s";
  }

  return 0;
}

void ConfigParser::UpdateRemainingString(std::string& remaining, size_t consumed_chars)
{
  remaining = remaining.substr(consumed_chars);
  remaining = libft::FT_Trim(remaining);
}

void ConfigParser::ValidateUnitOrder(const std::string& last_unit, const std::string& current_unit)
{
  if (last_unit.empty())
    return;

  bool valid_order = false;
  if (last_unit == "d")
  {
    valid_order = (current_unit == "h" || current_unit == "m" || current_unit == "s" || current_unit == "ms");
  }
  else if (last_unit == "h")
  {
    valid_order = (current_unit == "m" || current_unit == "s" || current_unit == "ms");
  }
  else if (last_unit == "m")
  {
    valid_order = (current_unit == "s" || current_unit == "ms");
  }
  else if (last_unit == "s")
  {
    valid_order = (current_unit == "ms");
  }

  if (!valid_order)
  {
    throw std::runtime_error("\"keepalive_timeout\" directive invalid value");
  }
}

time_t ConfigParser::ConvertToMilliseconds(time_t value, const std::string& unit)
{
  if (unit == "d")
    return value * 24 * 60 * 60 * 1000LL;
  else if (unit == "h")
    return value * 60 * 60 * 1000LL;
  else if (unit == "m")
    return value * 60 * 1000LL;
  else if (unit == "s")
    return value * 1000LL;
  else if (unit == "ms")
    return value;
  else
    throw std::runtime_error("\"keepalive_timeout\" directive invalid value");
}

void ConfigParser::ValidateOverflow(time_t current_total, time_t value_to_add)
{
  if (value_to_add > 9223372036854775807LL - current_total)
  {
    throw std::runtime_error("\"keepalive_timeout\" directive invalid value");
  }
}

std::string ConfigParser::ParseLocationPath()
{
  std::string path = ExtractLocationPathFromCurrentLine();
  FindLocationBlockOpening();
  return path;
}

std::string ConfigParser::ExtractLocationPathFromCurrentLine()
{
  std::string::size_type first_space = _current_line.find(' ');
  std::string path;

  if (first_space != std::string::npos)
  {
    path = _current_line.substr(0, first_space);
    _current_line = _current_line.substr(first_space + 1);
  }
  else
  {
    path = _current_line;
    std::getline(_input, _current_line);
  }

  return path;
}

void ConfigParser::FindLocationBlockOpening()
{
  while (true)
  {
    _current_line = libft::FT_Trim(_current_line);
    if (_current_line.empty())
    {
      if (!std::getline(_input, _current_line))
      {
        throw std::runtime_error("unexpected end of file");
      }
      continue;
    }

    if (_current_line[0] == '{')
    {
      _current_line = libft::FT_Trim(_current_line.substr(1));
      break;
    }
    else
    {
      throw std::runtime_error("invalid location block format");
    }
  }
}

void ConfigParser::ParseLocationRedirectDirective(const std::string &value,
                                                  LocationConfig *location)
{
  if (!location->GetRedirect().first.empty() &&
      location->GetRedirect().second != -1)
    return;
  const Redirection &redirect = ParseRedirectValue(value);
  location->SetRedirect(redirect.first, redirect.second);
}

void ConfigParser::ParseCgiPassDirective(const std::string &value, LocationConfig *location)
{
  std::string remaining = value;
  std::string extension = parsing_utils::GetNextToken(remaining);
  std::string executor = parsing_utils::GetNextToken(remaining);
  std::string extra = parsing_utils::GetNextToken(remaining);

  if (extension.empty() || executor.empty() || !extra.empty())
  {
    throw std::runtime_error("invalid number of arguments in \"cgi_pass\" directive");
  }

  if (extension != ".php")
  {
    throw std::runtime_error("this server only supports PHP CGI (.php extension)");
  }

  location->AddCgiExecutor(extension, executor);
}

void ConfigParser::ParseCgiReadTimeoutDirective(const std::string &value, LocationConfig *location)
{
  std::string remaining = value;
  remaining = parsing_utils::ExtractQuotedString(remaining);
  if (remaining.empty())
  {
    throw std::runtime_error("invalid number of arguments in \"cgi_read_timeout\" directive");
  }

  time_t timeout = ParseTimeout(remaining);
  location->SetCgiReadTimeout(timeout);
}
