#include "../../inc/Response/mime_type.h"

std::map<std::string, std::string>& MimeType::GetMimeTypes() {
  static std::map<std::string, std::string> mime_types;
  static bool initialized = false;

  if (!initialized) {
    mime_types["html"] = "text/html";
    mime_types["htm"] = "text/html";
    mime_types["css"] = "text/css";
    mime_types["js"] = "text/javascript";
    mime_types["txt"] = "text/plain";
    mime_types["jpg"] = "image/jpeg";
    mime_types["jpeg"] = "image/jpeg";
    mime_types["png"] = "image/png";
    mime_types["gif"] = "image/gif";
    mime_types["ico"] = "image/x-icon";
    mime_types["xml"] = "text/xml";
    mime_types["pdf"] = "application/pdf";
    mime_types["zip"] = "application/zip";
    mime_types["gz"] = "application/gzip";
    mime_types["json"] = "application/json";
    initialized = true;
  }
  return mime_types;
}

std::string MimeType::GetType(const std::string& extension) {
  std::map<std::string, std::string>::iterator it =
      GetMimeTypes().find(extension);
  if (it != GetMimeTypes().end()) {
    return it->second;
  }
  return "application/octet-stream"; // Default value for unknown file types metioned in the RFC 9110
}
