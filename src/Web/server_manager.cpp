#include "../../inc/Web/server_manager.h"

ServerManager::~ServerManager() {
  for (size_t i = 0; i < servers_.size(); ++i) {
    delete servers_[i];
  }

  servers_.clear();
  server_map_.clear();
}

ServerManager::ServerKey ServerManager::MakeServerKey(const std::string& host,
                                                      int port) {
  return std::make_pair(host, port);
}

void ServerManager::InitServers(const HttpConfig& config) {
  try {
    EpollHandler::Instance().Init(1024);
    CreateServerInstances(config);
    RegisterAndStartServers();
  } catch (const std::exception& e) {
    HandleInitException(e);
  }
}

void ServerManager::CreateServerInstances(const HttpConfig& config) {
  for (size_t i = 0; i < config.GetServers().size(); ++i) {
    const std::vector<ListenDirective>& lds = config.GetServers()[i]->GetListenDirectives();

    for (size_t j = 0; j < lds.size(); ++j) {
      const std::string& host = lds[j].host;
      int port = lds[j].port;
      ServerKey key = MakeServerKey(host, port);

      if (server_map_.find(key) == server_map_.end()) {
        server_map_[key] = new HttpServer(host, port);
      }

      server_map_[key]->AddServerConfig(config.GetServers()[i]);
    }
  }
}

void ServerManager::RegisterAndStartServers() {
  std::map<ServerKey, HttpServer*>::iterator it;
  for (it = server_map_.begin(); it != server_map_.end(); ++it) {
    EpollHandler::Instance().AddServer(it->second);
    it->second->Start();
    servers_.push_back(it->second);
  }
}

void ServerManager::HandleInitException(const std::exception& e) {
  std::cerr << "[ERROR] Exception during server initialization: " << e.what() << std::endl;
  for (std::map<ServerKey, HttpServer*>::iterator it = server_map_.begin();
       it != server_map_.end(); ++it) {
    delete it->second;
  }
  server_map_.clear();
  servers_.clear();
  throw;
}
