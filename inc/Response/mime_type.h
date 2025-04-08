#ifndef WEBSERV_INCLUDES_MIME_TYPE_H_
#define WEBSERV_INCLUDES_MIME_TYPE_H_

#include <map>
#include <string>

class MimeType {
 public:
  static std::string GetType(const std::string& extension);

 private:
  static std::map<std::string, std::string>& GetMimeTypes();
};

#endif
