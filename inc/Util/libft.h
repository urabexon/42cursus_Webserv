#ifndef WEBSERV_UTIL_H_
#define WEBSERV_UTIL_H_

#include <netinet/in.h>
#include <ctime>
#include <sstream>

namespace libft {
std::string FT_Trim(const std::string &str);
std::string FT_ToLower(const std::string &str);
std::string FT_ToUpper(const std::string &str);
std::string FT_StrCapitalize(const std::string &str);
std::string FT_GetGMTDate();
std::string FT_Ipv4ToString(const struct in_addr &inAddr);

}  // namespace libft

#endif  // WEBSERV_UTIL_H_
