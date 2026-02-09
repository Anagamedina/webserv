#include "HttpHeaderUtils.hpp"

#include <algorithm>
#include <cctype>

namespace http_header_utils {

std::string trimSpaces(const std::string& value) {
    std::string::size_type start = value.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    std::string::size_type end = value.find_last_not_of(" \t");
    return value.substr(start, end - start + 1);
}

std::string toLowerCopy(const std::string& value) {
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

bool splitHeaderLine(const std::string& line, std::string& key, std::string& value) {
    std::string::size_type colon = line.find(':');
    if (colon == std::string::npos)
        return false;
    key = line.substr(0, colon);
    value = trimSpaces(line.substr(colon + 1));
    return true;
}

} // namespace http_header_utils
