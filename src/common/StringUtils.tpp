#pragma once

#include <sstream>
#include <string>

namespace string_utils {

template <typename T>
std::string toString(const T& value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

}  // namespace string_utils
