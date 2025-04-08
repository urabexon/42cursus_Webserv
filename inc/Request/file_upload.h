#ifndef WEBSERV_inc_FILE_UPLOAD_H_
#define WEBSERV_inc_FILE_UPLOAD_H_

#include <vector>
#include <fstream>

class FileUpload {
 public:
  FileUpload();
  ~FileUpload();

  const std::string& GetFieldName() const;
  const std::string& GetFileName() const;
  const std::string& GetContentType() const;
  const std::vector<unsigned char>& GetContent() const;
  std::size_t GetSize() const;

  void SetFieldName(const std::string& field_name);
  void SetFileName(const std::string& file_name);
  void SetContentType(const std::string& content_type);
  void SetContent(const std::vector<unsigned char>& content);
  void SetContent(const std::string& content);
  bool SaveToFile(const std::string& upload_path) const;

 private:
  std::string field_name_;
  std::string file_name_;
  std::string content_type_;
  std::vector<unsigned char> content_;
  std::size_t size_;
};

#endif
