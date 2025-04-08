#include "../../inc/Request/file_upload.h"


FileUpload::FileUpload() : size_(0) {}

FileUpload::~FileUpload() {}

const std::string& FileUpload::GetFieldName() const { return field_name_; }

const std::string& FileUpload::GetFileName() const { return file_name_; }

const std::string& FileUpload::GetContentType() const { return content_type_; }

const std::vector<unsigned char>& FileUpload::GetContent() const {
  return content_;
}

std::size_t FileUpload::GetSize() const { return size_; }

void FileUpload::SetFieldName(const std::string& field_name) {
  field_name_ = field_name;
}

void FileUpload::SetFileName(const std::string& file_name) {
  file_name_ = file_name;
}

void FileUpload::SetContentType(const std::string& content_type) {
  content_type_ = content_type;
}

void FileUpload::SetContent(const std::vector<unsigned char>& content) {
  content_ = content;
  size_ = content.size();
}

void FileUpload::SetContent(const std::string& content) {
  content_.assign(content.begin(), content.end());
  size_ = content.size();
}

bool FileUpload::SaveToFile(const std::string& upload_path) const {
  if (file_name_.empty() || content_.empty()) {
    return false;
  }

  std::string full_path = upload_path;
  if (!upload_path.empty() && upload_path[upload_path.length() - 1] != '/') {
    full_path += "/";
  }
  full_path += file_name_;

  std::ofstream file(full_path.c_str(), std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  file.write(reinterpret_cast<const char*>(&content_[0]), content_.size());
  file.close();

  return true;
}
