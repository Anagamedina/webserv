#ifndef STRING_UTILS_TPP
#define STRING_UTILS_TPP

#include <sstream>
#include <string>

template <typename T>
std::string StringUtils::toString(const T& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

#endif
