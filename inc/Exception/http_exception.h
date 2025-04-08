#ifndef HTTP_EXCEPTION_H
#define HTTP_EXCEPTION_H

#include <stdexcept>

class HttpException : public std::runtime_error {
 protected:
  int status_;

 public:
  HttpException(int status, const std::string& message);
  virtual ~HttpException() throw();
  int getStatus() const;
};

class BadRequestException : public HttpException {
 public:
  explicit BadRequestException(const std::string& message = "");
  ~BadRequestException() throw();
};

class UriTooLongException : public HttpException {
 public:
  UriTooLongException();
  ~UriTooLongException() throw();
};

class NotImplementedException : public HttpException {
 public:
  NotImplementedException();
  ~NotImplementedException() throw();
};

class MethodNotAllowedException : public HttpException {
 public:
  MethodNotAllowedException();
  ~MethodNotAllowedException() throw();
};

class ContentTooLargeException : public HttpException {
 public:
  ContentTooLargeException();
  ~ContentTooLargeException() throw();
};

class UnsupportedMediaTypeException : public HttpException {
 public:
  UnsupportedMediaTypeException();
  ~UnsupportedMediaTypeException() throw();
};

class UnprocessableContentException : public HttpException {
 public:
  UnprocessableContentException();
  ~UnprocessableContentException() throw();
};

class InternalServerErrorException : public HttpException {
 public:
  InternalServerErrorException();
  ~InternalServerErrorException() throw();
};

class ServiceUnavailableException : public HttpException {
 public:
  ServiceUnavailableException();
  ~ServiceUnavailableException() throw();
};

class UnauthorizedException : public HttpException {
 public:
  UnauthorizedException();
  ~UnauthorizedException() throw();
};

class ForbiddenException : public HttpException {
 public:
  ForbiddenException();
  ~ForbiddenException() throw();
};

class NotFoundException : public HttpException {
 public:
  NotFoundException();
  ~NotFoundException() throw();
};

class ConflictException : public HttpException {
 public:
  ConflictException();
  ~ConflictException() throw();
};

class GoneException : public HttpException {
 public:
  GoneException();
  ~GoneException() throw();
};

class LengthRequiredException : public HttpException {
 public:
  LengthRequiredException();
  ~LengthRequiredException() throw();
};

class PreconditionFailedException : public HttpException {
 public:
  PreconditionFailedException();
  ~PreconditionFailedException() throw();
};

class MisdirectedRequestException : public HttpException {
 public:
  MisdirectedRequestException();
  ~MisdirectedRequestException() throw();
};

class BadGatewayException : public HttpException {
 public:
  BadGatewayException();
  ~BadGatewayException() throw();
};

class GatewayTimeoutException : public HttpException {
 public:
  GatewayTimeoutException();
  ~GatewayTimeoutException() throw();
};

class HttpVersionNotSupportedException : public HttpException {
 public:
  HttpVersionNotSupportedException();
  ~HttpVersionNotSupportedException() throw();
};

class RequestTimeoutException : public HttpException {
 public:
  RequestTimeoutException();
  ~RequestTimeoutException() throw();
};

class UpgradeRequiredException : public HttpException {
 public:
  UpgradeRequiredException();
  ~UpgradeRequiredException() throw();
};

#endif  // HTTP_EXCEPTION_H
