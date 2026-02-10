#include "StringUtils.hpp"

#include <cerrno>
#include <climits>
#include <cstdlib>

namespace string_utils {

// Explicit instantiations or specialized helpers if needed.
// Since toString is a template in header, we don't strictly need intToString
// wrapping it unless for convenience. But we declared stringToInt and
// stringToLong.

int stringToInt(const std::string& str, int defaultValue) {
  if (str.empty()) return defaultValue;

  const char* s = str.c_str();
  char* end;
  errno = 0;
  long result = std::strtol(s, &end, 10);

  if (end == s || *end != '\0' || errno == ERANGE || result > INT_MAX ||
      result < INT_MIN) {
    return defaultValue;
  }

  return static_cast<int>(result);
}

long stringToLong(const std::string& str, long defaultValue) {
  if (str.empty()) return defaultValue;

  const char* s = str.c_str();
  char* end;
  errno = 0;
  long result = std::strtol(s, &end, 10);

  if (end == s || *end != '\0' || errno == ERANGE) {
    return defaultValue;
  }

  return result;
}

}  // namespace string_utils
// Explicit template instantiations (to avoid linker errors)
// These tell the compiler to generate code for these types
// template std::string StringUtils::toString<int>(const int&);
// template std::string StringUtils::toString<long>(const long&);
// template std::string StringUtils::toString<unsigned int>(const unsigned
// int&); template std::string StringUtils::toString<unsigned long>(const
// unsigned long&); template std::string StringUtils::toString<short>(const
// short&); template std::string StringUtils::toString<float>(const float&);
// template std::string StringUtils::toString<double>(const double&);
