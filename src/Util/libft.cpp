#include "../../inc/Util/libft.h"

namespace libft {

std::string FT_Trim(const std::string& str) {
  std::string::size_type first = str.find_first_not_of(" \t");
  std::string::size_type last = str.find_last_not_of(" \t");
  return (first == std::string::npos) ? ""
                                      : str.substr(first, last - first + 1);
}

std::string FT_ToLower(const std::string& str) {
  std::string lower_str = str;
  for (std::string::iterator it = lower_str.begin(); it != lower_str.end(); ++it) {
    *it = std::tolower(*it);
  }
  return lower_str;
}

std::string FT_ToUpper(const std::string& str) {
  std::string upper_str = str;
  for (std::string::iterator it = upper_str.begin(); it != upper_str.end(); ++it) {
    *it = std::toupper(*it);
  }
  return upper_str;
}

std::string FT_StrCapitalize(const std::string& str) {
  std::string capitalized_str = str;
  for (std::string::size_type i = 0; i < capitalized_str.length(); ++i) {
    if (i == 0 || capitalized_str[i - 1] == ' ') {
      capitalized_str[i] = std::toupper(capitalized_str[i]);
    }
  }
  return capitalized_str;
}

std::string FT_GetGMTDate() {
  std::time_t current_time = std::time(0);
  char date_buffer[50];
  std::strftime(date_buffer, sizeof(date_buffer), "%a, %d %b %Y %H:%M:%S GMT",
                std::gmtime(&current_time));
  return std::string(date_buffer);
}

std::string FT_Ipv4ToString(const struct in_addr& inAddr) {
  unsigned long addr = ntohl(inAddr.s_addr);

  unsigned char b1 = (addr >> 24) & 0xFF;
  unsigned char b2 = (addr >> 16) & 0xFF;
  unsigned char b3 = (addr >> 8) & 0xFF;
  unsigned char b4 = addr & 0xFF;

  std::stringstream ss;
  ss << static_cast<unsigned int>(b1) << "." << static_cast<unsigned int>(b2)
     << "." << static_cast<unsigned int>(b3) << "."
     << static_cast<unsigned int>(b4);

  return ss.str();
}
}  // namespace libft
