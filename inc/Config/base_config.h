#ifndef BASE_CONFIG_HPP
#define BASE_CONFIG_HPP

#include <map>
#include <vector>
#include <string>

class BaseConfig
{

protected:
  std::string root_;
  std::size_t client_max_body_size_;
  std::map<int, std::string> error_pages_;
  bool autoindex_;
  bool autoindex_set_;
  std::vector<std::string> index_files_;

public:
  BaseConfig();
  virtual ~BaseConfig();
  BaseConfig(const BaseConfig &other);
  BaseConfig &operator=(const BaseConfig &other);

  void SetRoot(const std::string &root);
  const std::string &GetRoot() const;
  void SetClientMaxBodySize(std::size_t size);
  std::size_t GetClientMaxBodySize() const;
  void AddErrorPage(int code, const std::string &page);
  const std::map<int, std::string> &GetErrorPages() const;
  void SetAutoindex(bool autoindex);
  bool GetAutoindex() const;
  bool IsAutoindexSet() const;
  void AddIndexFile(const std::string &index_file);
  const std::vector<std::string> &GetIndexFiles() const;
  void ClearIndexFiles();
};

#endif
