#include "../../inc/Request/multipart_data.h"

MultipartData::MultipartData() {}

MultipartData::~MultipartData() {}

void MultipartData::AddFile(const std::string& name,
                            const std::string& filename,
                            const std::string& content,
                            const std::string& content_type) {
  FileUpload file;
  file.SetFieldName(name);
  file.SetFileName(filename);
  file.SetContent(content);
  file.SetContentType(content_type);
  files_.push_back(file);
}

void MultipartData::AddField(const std::string& name,
                             const std::string& value) {
  fields_[name].push_back(value);
}

const std::vector<FileUpload>& MultipartData::GetFiles() const {
  return files_;
}

const std::map<std::string, std::vector<std::string> >&
MultipartData::GetFields() const {
  return fields_;
}
