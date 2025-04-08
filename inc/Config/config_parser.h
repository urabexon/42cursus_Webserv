#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <limits>
#include <fstream>

#include "../Util/parsing_utils.h"
#include "location_config.h"

typedef std::pair<std::string, std::string> Directive;
typedef std::pair<std::string, int> Redirection;

class ConfigParser
{
public:
  ConfigParser();
  ~ConfigParser();

  HttpConfig* Parse(const std::string &filename);

private:
  std::ifstream _input;
  std::string _current_line;
  std::string _directive_line;

  void ReadNextNonEmptyLine();
  bool IsBlockEnd();
  void JoinMultiLineDirective();

  void OpenConfigFile(const std::string &filename);
  void FindAndParseHttpBlock(HttpConfig* http_config);
  void CheckAndCreateDefaultServer(HttpConfig* http_config);

  void ParseHttpBlock(HttpConfig *http_config);
  void ParseServerBlock(ServerConfig *server_config);
  void ParseLocationBlock(LocationConfig *location);

  void CreateDefaultServerIfNeeded(HttpConfig *http_config);
  void ParseServerBlockEntry(HttpConfig *http_config);
  void ParseLocationBlockEntry(ServerConfig *server_config);
  void SetServerDefaults(ServerConfig *server_config);
  void SetLocationDefaults(LocationConfig *location_config);
  void HandleHttpDirectiveLine(HttpConfig *http_config);

  void HandleHttpDirective(HttpConfig *http_config);
  void HandleServerDirective(ServerConfig *server);
  void HandleLocationDirective(LocationConfig *location);
  void HandleCommonDirective(const Directive &directive, BaseConfig *config);

  void ParseClientMaxBodySizeDirective(const std::string &value, BaseConfig *config);
  void ParseErrorPageDirective(const std::string &value, BaseConfig *config);
  void ParseAutoIndexDirective(const std::string &value, BaseConfig *config);
  void ParseIndexDirective(const std::string &value, BaseConfig *config);
  void ParseKeepaliveTimeoutDirective(const std::string &value, BaseConfig *config);
  void ParseServerRedirectDirective(const std::string &value, BaseConfig *config);
  void ParseListenDirective(const std::string &value, ServerConfig *server);
  void ParseServerNameDirective(const std::string &value, ServerConfig *server);
  void ParseLimitExceptDirective(const std::string &value, LocationConfig *location);
  void ParseLocationRedirectDirective(const std::string &value, LocationConfig *location);
  void ParseCgiPassDirective(const std::string &value, LocationConfig *location);
  void ParseCgiReadTimeoutDirective(const std::string &value, LocationConfig *location);

  Directive ExtractDirective();
  Redirection ParseRedirectValue(const std::string &value);
  std::string ResolveHost(const std::string &host_part);
  int ParsePort(const std::string &port_str);
  void validateDuplicatedAddress(const std::string &host, int port, ServerConfig *server);
  time_t ParseTimeout(const std::string &timeout_str);
  std::string ParseLocationPath();

  void ExtractValueAndUnit(std::string& remaining, time_t& value, std::string& unit);
  void ValidateUnitOrder(const std::string& last_unit, const std::string& current_unit);
  time_t ConvertToMilliseconds(time_t value, const std::string& unit);
  void ValidateOverflow(time_t current_total, time_t value_to_add);

  void ParseListenAddressPart(const std::string &value, std::string &host, int &port, bool &is_default);
  void ParseHostAndPort(const std::string &addr_part, std::string &host, int &port);
  bool IsDigitsOnly(const std::string &str);
  void ParseListenOptions(std::string &remaining, bool &is_default);

  void ExtractUntilSemicolon();
  Directive SeparateDirectiveAndValue();
  size_t FindFirstSpaceOutsideQuotes();

  void ExtractSizeAndMultiplier(const std::string &value, std::string &number_str, off_t &multiplier);
  void ValidateNumericValue(const std::string &number_str);
  off_t ParseSizeValue(const std::string &number_str, off_t multiplier);

  std::string ExtractLocationPathFromCurrentLine();
  void FindLocationBlockOpening();

  void SetupLocationPath(LocationConfig *location_config);
  bool ReadNextLocationLine();
  void ParseLocationDirectives(LocationConfig *location_config);

  bool IsIPv4Address(const std::string &host_part);
  std::string ValidateIPv4Address(const std::string &host_part);
  std::string ResolveHostname(const std::string &host_part);

  std::string ExtractPortPart(const std::string &port_str);
  void ValidatePortString(const std::string &port_part, const std::string &port_str);
  int ConvertToPortNumber(const std::string &port_part, const std::string &port_str);

  std::string ExtractNumericPart(const std::string& remaining);
  time_t ParseTimeValue(const std::string& num_str);
  size_t ExtractTimeUnit(const std::string& remaining, size_t start_pos, std::string& unit);
  void UpdateRemainingString(std::string& remaining, size_t consumed_chars);
};

#endif
