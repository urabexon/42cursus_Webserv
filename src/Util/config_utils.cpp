#include "../../inc/Util/config_utils.h"


std::string ExtractHostAndPort(const HttpRequest& request, int* port_out, bool* port_in_host_out) {
  std::string host;
  *port_out = -1;
  *port_in_host_out = false;

  std::map<std::string, std::string>::const_iterator it =
      request.GetHeaders().find("host");
  if (it == request.GetHeaders().end()) {
    return host;
  }

  host = it->second;
  size_t colon_pos = host.find(':');
  if (colon_pos != std::string::npos) {
    std::string port_str = host.substr(colon_pos + 1);
    std::istringstream iss(port_str);
    iss >> *port_out;
    *port_in_host_out = true;
    host = host.substr(0, colon_pos);
  }

  return host;
}

bool CheckServerNameMatch(const std::string& server_name_group, const std::string& host) {
  size_t pos = 0;
  size_t prev_pos = 0;

  while ((pos = server_name_group.find(' ', prev_pos)) != std::string::npos) {
    std::string individual_name = server_name_group.substr(prev_pos, pos - prev_pos);
    if (!individual_name.empty() && individual_name == host) {
      return true;
    }
    prev_pos = pos + 1;
  }

  std::string last_name = server_name_group.substr(prev_pos);
  return (!last_name.empty() && last_name == host);
}

ServerConfig* FindServerByPriority(
    ServerConfig* name_port_match,
    ServerConfig* default_server_on_port,
    ServerConfig* first_server_on_port,
    ServerConfig* exact_name_match,
    ServerConfig* first_server) {

  if (name_port_match) {
    return name_port_match;
  }

  if (default_server_on_port) {
    return default_server_on_port;
  }

  if (first_server_on_port) {
    return first_server_on_port;
  }

  if (exact_name_match) {
    return exact_name_match;
  }

  return first_server;
}

void FindServerCandidates(
    const HttpConfig* config,
    const std::string& host,
    int request_port,
    ServerConfig** exact_name_match,
    ServerConfig** name_port_match,
    ServerConfig** default_server_on_port,
    ServerConfig** first_server_on_port,
    ServerConfig** first_server)
{
    *exact_name_match = NULL;
    *name_port_match = NULL;
    *default_server_on_port = NULL;
    *first_server_on_port = NULL;
    *first_server = NULL;

    for (std::vector<ServerConfig*>::const_iterator it = config->GetServers().begin();
         it != config->GetServers().end(); ++it) {
        if (!*first_server) {
            *first_server = *it;
        }

        ProcessServerForMatching(*it, host, request_port,
                               exact_name_match, name_port_match,
                               default_server_on_port, first_server_on_port);
    }
}

void ProcessServerForMatching(
    ServerConfig* server,
    const std::string& host,
    int request_port,
    ServerConfig** exact_name_match,
    ServerConfig** name_port_match,
    ServerConfig** default_server_on_port,
    ServerConfig** first_server_on_port)
{
    bool port_matches = CheckPortMatch(server, request_port,
                                     default_server_on_port, first_server_on_port);

    CheckNameMatch(server, host, port_matches,
                 name_port_match, exact_name_match);
}

bool CheckPortMatch(
    ServerConfig* server,
    int request_port,
    ServerConfig** default_server_on_port,
    ServerConfig** first_server_on_port)
{
    const std::vector<ListenDirective>& listen_directives = server->GetListenDirectives();

    for (std::vector<ListenDirective>::const_iterator listen_it = listen_directives.begin();
         listen_it != listen_directives.end(); ++listen_it) {
        if (listen_it->port == request_port) {
            if (!*first_server_on_port) {
                *first_server_on_port = server;
            }
            if (server->IsDefault() && !*default_server_on_port) {
                *default_server_on_port = server;
            }
            return true;
        }
    }
    return false;
}

bool CheckNameMatch(
    ServerConfig* server,
    const std::string& host,
    bool port_matches,
    ServerConfig** name_port_match,
    ServerConfig** exact_name_match)
{
    const std::vector<std::string>& server_names = server->GetServerNames();

    for (std::vector<std::string>::const_iterator name_it = server_names.begin();
         name_it != server_names.end(); ++name_it) {
        if (CheckServerNameMatch(*name_it, host)) {
            if (port_matches) {
                *name_port_match = server;
            } else if (!*exact_name_match) {
                *exact_name_match = server;
            }
            return true;
        }
    }
    return false;
}

ServerConfig* FindMatchingServerConfig(
    const HttpRequest& request,
    const HttpConfig* config) {
  if (config->GetServers().empty()) {
    throw InternalServerErrorException();
  }

  int request_port;
  bool port_in_host;
  std::string host = ExtractHostAndPort(request, &request_port, &port_in_host);

  if (!port_in_host) {
    request_port = request.GetPort();
  }

  ServerConfig* exact_name_match;
  ServerConfig* name_port_match;
  ServerConfig* default_server_on_port;
  ServerConfig* first_server_on_port;
  ServerConfig* first_server;

  FindServerCandidates(config, host, request_port,
                     &exact_name_match, &name_port_match,
                     &default_server_on_port, &first_server_on_port,
                     &first_server);

  return FindServerByPriority(
      name_port_match,
      default_server_on_port,
      first_server_on_port,
      exact_name_match,
      first_server);
}

const LocationConfig* FindExactLocationMatch(
    const std::map<std::string, LocationConfig*>& locations,
    const std::string& request_path) {
  std::map<std::string, LocationConfig*>::const_iterator it = locations.find(request_path);
  if (it != locations.end()) {
    return it->second;
  }
  return NULL;
}

const LocationConfig* FindPrefixLocationMatch(
    const std::map<std::string, LocationConfig*>& locations,
    const std::string& request_path) {
  const LocationConfig* prefix_match = NULL;
  size_t longest_match = 0;

  for (std::map<std::string, LocationConfig*>::const_iterator it = locations.begin();
       it != locations.end(); ++it) {
    const std::string& location_path = it->first;

    if (request_path.compare(0, location_path.length(), location_path) == 0) {
      if (location_path.length() > longest_match) {
        longest_match = location_path.length();
        prefix_match = it->second;
      }
    }
  }

  return prefix_match;
}

const LocationConfig* FindRootLocation(
    const std::map<std::string, LocationConfig*>& locations) {
  std::map<std::string, LocationConfig*>::const_iterator it = locations.find("/");
  if (it != locations.end()) {
    return it->second;
  }
  return NULL;
}

const LocationConfig* FindMatchingLocation(
    const HttpRequest& request,
    const ServerConfig* config) {
  if (!config) {
    return NULL;
  }

  const std::map<std::string, LocationConfig*>& locations = config->GetLocations();
  const std::string& request_path = request.GetPath();

  const LocationConfig* exact_match = FindExactLocationMatch(locations, request_path);
  if (exact_match) return exact_match;

  const LocationConfig* prefix_match = FindPrefixLocationMatch(locations, request_path);
  if (prefix_match) return prefix_match;

  return FindRootLocation(locations);
}
