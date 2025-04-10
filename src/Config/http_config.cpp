#include "../../inc/Config/http_config.h"
#include <iostream>

HttpConfig::HttpConfig() : keepalive_timeout_(75000), keepalive_timeout_set_(false) {
}

HttpConfig::HttpConfig(const HttpConfig& other) : BaseConfig(other), keepalive_timeout_(other.keepalive_timeout_) {
    for (std::vector<ServerConfig*>::const_iterator it = other.servers_.begin();
         it != other.servers_.end(); ++it) {
        servers_.push_back(new ServerConfig(**it));
    }
    autoindex_set_ = false;
    keepalive_timeout_set_ = false;
}

HttpConfig::~HttpConfig() {
    for (std::vector<ServerConfig*>::iterator it = servers_.begin();
         it != servers_.end(); ++it) {
        delete *it;
    }
}

HttpConfig& HttpConfig::operator=(const HttpConfig& other) {
    if (this != &other) {
        BaseConfig::operator=(other);
        for (std::vector<ServerConfig*>::iterator it = servers_.begin();
             it != servers_.end(); ++it) {
            delete *it;
        }
        servers_.clear();
        for (std::vector<ServerConfig*>::const_iterator it = other.servers_.begin();
             it != other.servers_.end(); ++it) {
            servers_.push_back(new ServerConfig(**it));
        }
        keepalive_timeout_ = other.keepalive_timeout_;
        keepalive_timeout_set_ = other.keepalive_timeout_set_;
    }
    autoindex_set_ = false;
    return *this;
}

const std::vector<ServerConfig*>& HttpConfig::GetServers() const {
    return servers_;
}

void HttpConfig::AddServer(ServerConfig* server) {
    if (HasServerNameConflict(server)) {
        PrintServerNameConflictWarning(server);
        delete server;
        return;
    }
    servers_.push_back(server);
}

time_t HttpConfig::GetKeepaliveTimeout() const {
    return keepalive_timeout_;
}

void HttpConfig::SetKeepaliveTimeout(time_t timeout) {
    keepalive_timeout_ = timeout;
    keepalive_timeout_set_ = true;
}

bool HttpConfig::IsKeepaliveTimeoutSet() const {
    return keepalive_timeout_set_;
}

bool HttpConfig::HasServerNameConflict(const ServerConfig* server) const {
    for (std::vector<ServerConfig*>::const_iterator it = servers_.begin(); it != servers_.end(); ++it) {
        if (*it == server) continue;

        if (HasConflictBetweenServers(server, *it)) {
            return true;
        }
    }
    return false;
}

bool HttpConfig::HasConflictBetweenServers(const ServerConfig* new_server,
                                          const ServerConfig* existing_server) const {
    const std::vector<ListenDirective>& new_listens = new_server->GetListenDirectives();
    const std::vector<ListenDirective>& existing_listens = existing_server->GetListenDirectives();

    for (std::vector<ListenDirective>::const_iterator new_listen = new_listens.begin();
         new_listen != new_listens.end(); ++new_listen) {
        for (std::vector<ListenDirective>::const_iterator existing_listen = existing_listens.begin();
             existing_listen != existing_listens.end(); ++existing_listen) {
            if (AreListenDirectivesOverlapping(*new_listen, *existing_listen) &&
                DoServerNamesConflict(new_server, existing_server)) {
                return true;
            }
        }
    }
    return false;
}

bool HttpConfig::AreListenDirectivesOverlapping(const ListenDirective& first,
                                               const ListenDirective& second) const {
    if (first.port != second.port) {
        return false;
    }

    if (first.host != second.host &&
        first.host != "0.0.0.0" &&
        second.host != "0.0.0.0") {
        return false;
    }

    return true;
}

bool HttpConfig::DoServerNamesConflict(const ServerConfig* first_server,
                                      const ServerConfig* second_server) const {
    const std::vector<std::string>& first_names = first_server->GetServerNames();
    const std::vector<std::string>& second_names = second_server->GetServerNames();

    if (first_names.empty() && second_names.empty()) {
        return true;
    }

    for (std::vector<std::string>::const_iterator first_name = first_names.begin();
         first_name != first_names.end(); ++first_name) {
        for (std::vector<std::string>::const_iterator second_name = second_names.begin();
             second_name != second_names.end(); ++second_name) {
            if (*first_name == *second_name) {
                return true;
            }
        }
    }
    return false;
}

void HttpConfig::PrintServerNameConflictWarning(const ServerConfig* server) const {
    const std::vector<std::string>& names = server->GetServerNames();
    const std::vector<ListenDirective>& listens = server->GetListenDirectives();

    std::string server_name = names.empty() ? "" : names[0];

    for (std::vector<ListenDirective>::const_iterator listen = listens.begin();
         listen != listens.end(); ++listen) {
        std::string host = listen->host.empty() ? "0.0.0.0" : listen->host;
        std::cerr << "[warn] conflicting server name \"" << (server_name == "_" ? "" : server_name)
                  << "\" on " << host << ":" << listen->port << ", ignored" << std::endl;
    }
}
