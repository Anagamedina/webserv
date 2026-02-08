#pragma once

#include <string>

namespace string_utils {

// Generic template for converting to string
template < typename T >
static std::string toString(const T& value);

int stringToInt(const std::string& str, int defaultValue = 0);
long stringToLong(const std::string& str, long defaultValue = 0);

}

#include "StringUtils.tpp"
