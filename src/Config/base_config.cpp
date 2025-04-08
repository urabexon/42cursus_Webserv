#include "../../inc/Config/base_config.h"

BaseConfig::BaseConfig() : client_max_body_size_(1024 * 1024), autoindex_(false), autoindex_set_(false) {}

BaseConfig::~BaseConfig() {}

BaseConfig::BaseConfig(const BaseConfig& other) {
  *this = other;
}

BaseConfig& BaseConfig::operator=(const BaseConfig& other) {
  if (this != &other) {
    root_ = other.root_;
    client_max_body_size_ = other.client_max_body_size_;
    autoindex_ = other.autoindex_;
    autoindex_set_ = other.autoindex_set_;
    error_pages_ = other.error_pages_;
    index_files_ = other.index_files_;
  }
  return *this;
}

void BaseConfig::SetRoot(const std::string& root) {
  root_ = root;
}

const std::string& BaseConfig::GetRoot() const {
  return root_;
}

void BaseConfig::SetClientMaxBodySize(std::size_t size) {
  client_max_body_size_ = size;
}

std::size_t BaseConfig::GetClientMaxBodySize() const {
  return client_max_body_size_;
}

void BaseConfig::AddErrorPage(int code, const std::string& page) {
  error_pages_[code] = page;
}

const std::map<int, std::string>& BaseConfig::GetErrorPages() const {
  return error_pages_;
}

void BaseConfig::SetAutoindex(bool autoindex) {
  autoindex_ = autoindex;
  autoindex_set_ = true;
}

bool BaseConfig::GetAutoindex() const {
  return autoindex_;
}

bool BaseConfig::IsAutoindexSet() const {
  return autoindex_set_;
}

void BaseConfig::AddIndexFile(const std::string& index_file) {
  index_files_.push_back(index_file);
}

const std::vector<std::string>& BaseConfig::GetIndexFiles() const {
  return index_files_;
}

void BaseConfig::ClearIndexFiles() {
  index_files_.clear();
}
