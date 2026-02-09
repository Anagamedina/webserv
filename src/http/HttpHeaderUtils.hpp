#ifndef HTTP_HEADER_UTILS_HPP
#define HTTP_HEADER_UTILS_HPP

#include <string>

namespace http_header_utils {

std::string trimSpaces(const std::string& value);
std::string toLowerCopy(const std::string& value);
bool splitHeaderLine(const std::string& line, std::string& key, std::string& value);

} // namespace http_header_utils

#endif // HTTP_HEADER_UTILS_HPP
