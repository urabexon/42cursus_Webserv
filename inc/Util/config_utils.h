#ifndef CONFIG_UTILS_H_
#define CONFIG_UTILS_H_

#include "../Request/http_request.h"
#include "../Config/server_config.h"
#include "../Config/http_config.h"
#include "../Exception/http_exception.h"

std::string ExtractHostAndPort(const HttpRequest& request, int* port_out, bool* port_in_host_out);
bool CheckServerNameMatch(const std::string& server_name_group, const std::string& host);
ServerConfig* FindServerByPriority(
    ServerConfig* name_port_match,
    ServerConfig* default_server_on_port,
    ServerConfig* first_server_on_port,
    ServerConfig* exact_name_match,
    ServerConfig* first_server);

void FindServerCandidates(
    const HttpConfig* config,
    const std::string& host,
    int request_port,
    ServerConfig** exact_name_match,
    ServerConfig** name_port_match,
    ServerConfig** default_server_on_port,
    ServerConfig** first_server_on_port,
    ServerConfig** first_server);

void ProcessServerForMatching(
    ServerConfig* server,
    const std::string& host,
    int request_port,
    ServerConfig** exact_name_match,
    ServerConfig** name_port_match,
    ServerConfig** default_server_on_port,
    ServerConfig** first_server_on_port);

bool CheckPortMatch(
    ServerConfig* server,
    int request_port,
    ServerConfig** default_server_on_port,
    ServerConfig** first_server_on_port);

bool CheckNameMatch(
    ServerConfig* server,
    const std::string& host,
    bool port_matches,
    ServerConfig** name_port_match,
    ServerConfig** exact_name_match);

ServerConfig* FindMatchingServerConfig(
    const HttpRequest& request,
    const HttpConfig* config);

const LocationConfig* FindExactLocationMatch(
    const std::map<std::string, LocationConfig*>& locations,
    const std::string& request_path);
const LocationConfig* FindPrefixLocationMatch(
    const std::map<std::string, LocationConfig*>& locations,
    const std::string& request_path);
const LocationConfig* FindRootLocation(
    const std::map<std::string, LocationConfig*>& locations);

const LocationConfig* FindMatchingLocation(
    const HttpRequest& request,
    const ServerConfig* config);

#endif
