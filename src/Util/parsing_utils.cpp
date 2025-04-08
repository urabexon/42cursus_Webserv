#include "../../inc/Util/parsing_utils.h"

namespace parsing_utils {

std::string GetNextToken(std::string& str) {
	if (str.empty()) {
		return "";
	}

	str = libft::FT_Trim(str);
	std::string token;

	if (str[0] == '"' || str[0] == '\'') {
		return ExtractQuotedString(str);
	}

	size_t pos = str.find_first_of(" \t");
	if (pos == std::string::npos) {
		token = str;
		str = "";
	} else {
		token = str.substr(0, pos);
		str = str.substr(pos);
		pos = str.find_first_not_of(" \t");
		if (pos != std::string::npos) {
			str = str.substr(pos);
		} else {
			str = "";
		}
	}
	return token;
}

std::string ExtractQuotedString(std::string& str) {
    if (str.empty()) {
        return str;
    }

    str = libft::FT_Trim(str);
    if (str[0] != '"' && str[0] != '\'') {
        return str;
    }

    if (str.length() == 1) {
        throw std::runtime_error("Empty quoted string");
    }

    char quote_char = str[0];
    str = str.substr(1);

    size_t quote_end = str.find(quote_char);
    if (quote_end == std::string::npos) {
        throw std::runtime_error("Unclosed quote");
    }

    std::string result = str.substr(0, quote_end);
    str = str.substr(quote_end + 1);

    return result;
}

bool IsInQuote(bool current_state, char current_char, char& quote_char, bool is_escaped) {
    if ((current_char == '"' || current_char == '\'') && !is_escaped) {
        if (!current_state) {
            quote_char = current_char;
            return true;
        } else if (current_char == quote_char) {
            return false;
        }
    }
    return current_state;
}

bool IsDirectiveStart(bool in_directive, char current_char) {
    if (!in_directive && !std::isspace(current_char) &&
        current_char != '{' && current_char != '}' && current_char != ';') {
        return true;
    }
    return false;
}

bool IsAfterDirective(bool in_directive, bool current_after_directive, char current_char) {
    if (in_directive && !current_after_directive && std::isspace(current_char)) {
        return true;
    }
    return false;
}

bool IsDirectiveEnd(char current_char) {
    return (current_char == ';' || current_char == '{' || current_char == '}');
}

bool IsValidComment(const std::string& str, size_t pos, bool in_quote, bool after_directive) {
    if (str[pos] != '#' || in_quote) {
        return false;
    }

    if (pos == 0 || std::isspace(str[pos-1]) || str[pos-1] == ';' ||
        str[pos-1] == '{' || str[pos-1] == '}') {
        return true;
    }

    if (after_directive && !std::isspace(str[pos-1]) &&
        str[pos-1] != ';' && str[pos-1] != '{' && str[pos-1] != '}') {
        return false;
    }

    throw std::runtime_error("invalid number of arguments");
}

bool ProcessCharacter(
    std::string& result,
    const std::string& str,
    size_t i,
    bool in_quote,
    bool after_directive)
{
    if (str[i] == '#') {
        if (IsValidComment(str, i, in_quote, after_directive)) {
            return true;
        }
    }

    result += str[i];
    return false;
}

void UpdateState(
    const std::string& str,
    size_t i,
    bool& in_quote,
    bool& in_directive,
    bool& after_directive,
    char& quote_char)
{
    bool is_escaped = (i > 0 && str[i-1] == '\\');

    in_quote = IsInQuote(in_quote, str[i], quote_char, is_escaped);

    if (!in_quote) {
        if (IsDirectiveStart(in_directive, str[i])) {
            in_directive = true;
        } else if (IsAfterDirective(in_directive, after_directive, str[i])) {
            after_directive = true;
        } else if (IsDirectiveEnd(str[i])) {
            in_directive = false;
            after_directive = false;
        }
    }
}

std::string RemoveComments(const std::string& str) {
    std::string result;
    bool in_quote = false;
    bool in_directive = false;
    bool after_directive = false;
    char quote_char = 0;

    for (size_t i = 0; i < str.length(); ++i) {
        UpdateState(str, i, in_quote, in_directive, after_directive, quote_char);

        if (ProcessCharacter(result, str, i, in_quote, after_directive)) {
            break;
        }
    }

    return result;
}

std::string UrlDecode(const std::string& str) {
    std::string result;
    std::string::size_type i;
    for (i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                int value;
                std::istringstream is(str.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

}  // namespace parsing_utils
