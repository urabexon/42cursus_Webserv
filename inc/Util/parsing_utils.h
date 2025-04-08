#ifndef PARSING_UTILS_H_
#define PARSING_UTILS_H_

#include "libft.h"

namespace parsing_utils {

std::string GetNextToken(std::string& str);
std::string ExtractQuotedString(std::string& str);

bool IsInQuote(bool current_state, char current_char, char& quote_char, bool is_escaped);
bool IsDirectiveStart(bool in_directive, char current_char);
bool IsAfterDirective(bool in_directive, bool current_after_directive, char current_char);
bool IsDirectiveEnd(char current_char);
bool IsValidComment(const std::string& str, size_t pos, bool in_quote, bool after_directive);

bool ProcessCharacter(
    std::string& result,
    const std::string& str,
    size_t i,
    bool in_quote,
    bool after_directive);

void UpdateState(
    const std::string& str,
    size_t i,
    bool& in_quote,
    bool& in_directive,
    bool& after_directive,
    char& quote_char);

std::string RemoveComments(const std::string& str);

std::string UrlDecode(const std::string& str);

}  // namespace parsing_utils

#endif  // PARSING_UTILS_H_
