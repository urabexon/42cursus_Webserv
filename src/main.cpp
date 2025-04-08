#include "../inc/Config/config_parser.h"
#include "../inc/Web/server_manager.h"
#include "../inc/Web/epoll_handler.h"

#include <iostream>

void PrintServerNames(const std::vector<std::string>& server_names) {
  std::cout << "Server Names: ";
  for (std::vector<std::string>::const_iterator it = server_names.begin();
       it != server_names.end(); ++it) {
    if (it != server_names.begin()) {
      std::cout << ", ";
    }
    std::cout << *it;
  }
  std::cout << std::endl;
}

void PrintIndexFiles(const std::vector<std::string>& index_files) {
  if (index_files.empty()) {
    return;
  }

  std::cout << "Index Files: ";
  for (std::vector<std::string>::const_iterator it = index_files.begin();
       it != index_files.end(); ++it) {
    if (it != index_files.begin()) {
      std::cout << ", ";
    }
    std::cout << *it;
  }
  std::cout << std::endl;
}

void PrintListenDirectives(const std::vector<ListenDirective>& directives) {
  std::cout << "Listen Directives: ";
  for (std::vector<ListenDirective>::const_iterator it = directives.begin();
       it != directives.end(); ++it) {
    if (it != directives.begin()) {
      std::cout << ", ";
    }
    std::cout << it->host << ":" << it->port;
  }
  std::cout << std::endl;
}

void PrintErrorPages(const std::map<int, std::string>& error_pages) {
  if (error_pages.empty()) {
    return;
  }

  std::cout << "\nError Pages:" << std::endl;
  for (std::map<int, std::string>::const_iterator ep = error_pages.begin();
       ep != error_pages.end(); ++ep) {
    std::cout << "  " << ep->first << ": " << ep->second << std::endl;
  }
}

void PrintRedirect(const std::pair<std::string, int>& redirect) {
  if (redirect.second != -1) {
    std::cout << "Redirect: " << redirect.second << " " << redirect.first << std::endl;
  }
}

void PrintServerConfig(const ServerConfig *config) {
  std::cout << "\n=== Server Configuration ===" << std::endl;

  PrintServerNames(config->GetServerNames());
  std::cout << "Root Directory: " << config->GetRoot() << std::endl;
  std::cout << "Client Max Body Size: "
            << config->GetClientMaxBodySize() / (1024 * 1024) << "MB"
            << std::endl;
  std::cout << "Default Server: " << (config->IsDefault() ? "Yes" : "No")
            << std::endl;
  std::cout << "Keepalive Timeout: " << config->GetKeepaliveTimeout() << "ms"
            << std::endl;
  std::cout << "Autoindex: " << (config->GetAutoindex() ? "On" : "Off")
            << std::endl;

  PrintIndexFiles(config->GetIndexFiles());
  PrintListenDirectives(config->GetListenDirectives());
  PrintErrorPages(config->GetErrorPages());
  PrintRedirect(config->GetRedirect());
}

void PrintLocationBasicInfo(const LocationConfig *location) {
  std::cout << "\n  Path: " << location->GetPath() << std::endl;
  std::cout << "  Root: " << location->GetRoot() << std::endl;
  std::cout << "  Autoindex: " << (location->GetAutoindex() ? "On" : "Off") << std::endl;
  std::cout << "  Client Max Body Size: "
            << location->GetClientMaxBodySize() / (1024 * 1024) << "MB"
            << std::endl;
}

void PrintLocationIndexFiles(const std::vector<std::string>& index_files) {
  std::cout << "  Index Files: ";
  for (size_t i = 0; i < index_files.size(); ++i) {
    std::cout << index_files[i];
    if (i < index_files.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << std::endl;
}

void PrintLocationErrorPages(const std::map<int, std::string>& error_pages) {
  if (error_pages.empty()) {
    return;
  }

  std::cout << "  Error Pages:" << std::endl;
  for (std::map<int, std::string>::const_iterator it = error_pages.begin();
       it != error_pages.end(); ++it) {
    std::cout << "    " << it->first << ": " << it->second << std::endl;
  }
}

void PrintLocationCgiInfo(const LocationConfig *location) {
  const std::map<std::string, std::string>& cgi_executors = location->GetCgiExecutors();
  if (cgi_executors.empty()) {
    return;
  }

  std::cout << "  CGI Executors:" << std::endl;
  for (std::map<std::string, std::string>::const_iterator it = cgi_executors.begin();
       it != cgi_executors.end(); ++it) {
    std::cout << "    " << it->first << ": " << it->second << std::endl;
  }
  std::cout << "  CGI Read Timeout: " << location->GetCgiReadTimeout() << "ms" << std::endl;
}

void PrintLocationRedirect(const std::pair<std::string, int>& redirect) {
  if (redirect.second != -1) {
    std::cout << "  Redirection: " << redirect.second << " " << redirect.first << std::endl;
  }
}

void PrintLocationOptionalSettings(const LocationConfig *location) {
  if (!location->GetUploadPath().empty()) {
    std::cout << "  Upload Path: " << location->GetUploadPath() << std::endl;
  }

  if (!location->GetScriptFilename().empty()) {
    std::cout << "  Script Filename: " << location->GetScriptFilename() << std::endl;
  }
}

void PrintLocationMethods(const std::vector<std::string> &methods) {
  std::cout << "  Accepted Methods:";
  for (std::vector<std::string>::const_iterator m = methods.begin();
       m != methods.end(); ++m) {
    std::cout << " " << *m;
  }
  std::cout << std::endl;
}

void PrintLocationConfig(const LocationConfig *location) {
  PrintLocationBasicInfo(location);
  PrintLocationIndexFiles(location->GetIndexFiles());
  PrintLocationErrorPages(location->GetErrorPages());
  PrintLocationCgiInfo(location);
  PrintLocationRedirect(location->GetRedirect());
  PrintLocationOptionalSettings(location);
  PrintLocationMethods(location->GetAcceptedMethods());
}

void PrintFullServerConfig(ServerConfig *config) {
  PrintServerConfig(config);

  const std::map<std::string, LocationConfig *> &locations = config->GetLocations();
  if (!locations.empty()) {
    std::cout << "\nLocation Blocks:" << std::endl;
    for (std::map<std::string, LocationConfig *>::const_iterator it = locations.begin();
         it != locations.end(); ++it) {
      PrintLocationConfig(it->second);
    }
  }
  std::cout << "\n==========================\n" << std::endl;
}

void PrintHttpConfig(const HttpConfig& config) {
  std::cout << "\n=== HTTP Configuration ===" << std::endl;
  std::cout << "Root Directory: " << config.GetRoot() << std::endl;
  std::cout << "Client Max Body Size: " << config.GetClientMaxBodySize() / (1024 * 1024) << "MB" << std::endl;
  std::cout << "Keepalive Timeout: " << config.GetKeepaliveTimeout() << "ms" << std::endl;
  std::cout << "Autoindex: " << (config.GetAutoindex() ? "On" : "Off") << std::endl;

  const std::vector<std::string>& index_files = config.GetIndexFiles();
  if (!index_files.empty()) {
    std::cout << "Index Files: ";
    for (std::vector<std::string>::const_iterator it = index_files.begin(); it != index_files.end(); ++it) {
      if (it != index_files.begin()) {
        std::cout << ", ";
      }
      std::cout << *it;
    }
    std::cout << std::endl;
  }

  const std::map<int, std::string>& error_pages = config.GetErrorPages();
  if (!error_pages.empty()) {
    std::cout << "Error Pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it) {
      std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
  }

  std::cout << "\nServer Configurations:" << std::endl;
  const std::vector<ServerConfig*>& servers = config.GetServers();
  for (std::vector<ServerConfig*>::const_iterator it = servers.begin();
       it != servers.end(); ++it) {
    PrintFullServerConfig(*it);
  }
}

void PrintHttpRequest(const HttpRequest &dummy_request) {
  std::cout << dummy_request.GetMethod() << " " << dummy_request.GetPath()
            << " " << dummy_request.GetVersion() << std::endl;

  const std::map<std::string, std::string> &req_headers =
      dummy_request.GetHeaders();
  for (std::map<std::string, std::string>::const_iterator it =
           req_headers.begin();
       it != req_headers.end(); ++it) {
    std::cout << it->first << ": " << it->second << std::endl;
  }
  std::cout << std::endl;
  if (!dummy_request.GetBody().empty()) {
    std::cout << "Body: " << dummy_request.GetBody() << std::endl;
  }
}

int main(int argc, char** argv) {
  HttpConfig* config = NULL;
  try {
    ServerManager manager;
    ConfigParser parser;
    std::string config_path;
    if (argc > 1) {
        config_path = argv[1];
    } else {
        config_path = "./etc/webserv/webserv.conf";
    }

    config = parser.Parse(config_path);
    if(config != NULL && !config->GetServers().empty()){
      // PrintHttpConfig(*config);
      manager.InitServers(*config);

      EpollHandler::Instance().RunEventLoop();

      delete config;
    } else {
      delete config;
      std::cerr << "Error: No servers configured" << std::endl;
      return 1;
    }

  } catch (const std::exception &e) {
    delete config;
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
