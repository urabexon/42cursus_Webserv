#include "../../inc/Exception/http_exception.h"

HttpException::HttpException(int status, const std::string& message)
    : std::runtime_error(message), status_(status) {}

HttpException::~HttpException() throw() {}

int HttpException::getStatus() const { return status_; }

BadRequestException::BadRequestException(const std::string& message)
    : HttpException(400, message.empty() ? "Bad Request" : message) {}

BadRequestException::~BadRequestException() throw() {}

NotFoundException::NotFoundException() : HttpException(404, "Not Found") {}

NotFoundException::~NotFoundException() throw() {}

MethodNotAllowedException::MethodNotAllowedException()
    : HttpException(405, "Method Not Allowed") {}

MethodNotAllowedException::~MethodNotAllowedException() throw() {}

ContentTooLargeException::ContentTooLargeException()
    : HttpException(413, "Content Too Large") {}

ContentTooLargeException::~ContentTooLargeException() throw() {}

UriTooLongException::UriTooLongException()
    : HttpException(414, "URI Too Long") {}

UriTooLongException::~UriTooLongException() throw() {}

InternalServerErrorException::InternalServerErrorException()
    : HttpException(500, "Internal Server Error") {}

InternalServerErrorException::~InternalServerErrorException() throw() {}

NotImplementedException::NotImplementedException()
    : HttpException(501, "Not Implemented") {}

NotImplementedException::~NotImplementedException() throw() {}

ServiceUnavailableException::ServiceUnavailableException()
    : HttpException(503, "Service Unavailable") {}

ServiceUnavailableException::~ServiceUnavailableException() throw() {}

UnauthorizedException::UnauthorizedException()
    : HttpException(401, "Unauthorized") {}

UnauthorizedException::~UnauthorizedException() throw() {}

ForbiddenException::ForbiddenException() : HttpException(403, "Forbidden") {}

ForbiddenException::~ForbiddenException() throw() {}

ConflictException::ConflictException() : HttpException(409, "Conflict") {}

ConflictException::~ConflictException() throw() {}

GoneException::GoneException() : HttpException(410, "Gone") {}

GoneException::~GoneException() throw() {}

LengthRequiredException::LengthRequiredException()
    : HttpException(411, "Length Required") {}

LengthRequiredException::~LengthRequiredException() throw() {}

PreconditionFailedException::PreconditionFailedException()
    : HttpException(412, "Precondition Failed") {}

PreconditionFailedException::~PreconditionFailedException() throw() {}

MisdirectedRequestException::MisdirectedRequestException()
    : HttpException(421, "Misdirected Request") {}

MisdirectedRequestException::~MisdirectedRequestException() throw() {}

BadGatewayException::BadGatewayException()
    : HttpException(502, "Bad Gateway") {}

BadGatewayException::~BadGatewayException() throw() {}

GatewayTimeoutException::GatewayTimeoutException()
    : HttpException(504, "Gateway Timeout") {}

GatewayTimeoutException::~GatewayTimeoutException() throw() {}

HttpVersionNotSupportedException::HttpVersionNotSupportedException()
    : HttpException(505, "HTTP Version Not Supported") {}

HttpVersionNotSupportedException::~HttpVersionNotSupportedException() throw() {}

RequestTimeoutException::RequestTimeoutException()
    : HttpException(408, "Request Timeout") {}

RequestTimeoutException::~RequestTimeoutException() throw() {}

UpgradeRequiredException::UpgradeRequiredException()
    : HttpException(426, "Upgrade Required") {}

UpgradeRequiredException::~UpgradeRequiredException() throw() {}

UnprocessableContentException::UnprocessableContentException()
    : HttpException(422, "Unprocessable Content") {}

UnprocessableContentException::~UnprocessableContentException() throw() {}

UnsupportedMediaTypeException::UnsupportedMediaTypeException()
    : HttpException(415, "Unsupported Media Type") {}

UnsupportedMediaTypeException::~UnsupportedMediaTypeException() throw() {}
