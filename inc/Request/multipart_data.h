#ifndef WEBSERV_inc_MULTIPART_DATA_H_
#define WEBSERV_inc_MULTIPART_DATA_H_

#include <map>

#include "file_upload.h"

class MultipartData {
 public:
  MultipartData();
  ~MultipartData();

  void AddFile(const std::string& name, const std::string& filename,
               const std::string& content, const std::string& content_type);
  void AddField(const std::string& name, const std::string& value);
  const std::vector<FileUpload>& GetFiles() const;
  const std::map<std::string, std::vector<std::string> >& GetFields() const;

 private:
  std::vector<FileUpload> files_;
  std::map<std::string, std::vector<std::string> > fields_;
};

#endif
