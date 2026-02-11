#pragma once

#include <string>

class StringUtils {
 public:
  // Generic template for converting to string
  template <typename T>
  static std::string toString(const T& value);

  static int stringToInt(const std::string& str, int defaultValue = 0);

  static long stringToLong(const std::string& str, long defaultValue = 0);

 private:
  StringUtils();  // Prevent instantiation
  ~StringUtils();
  StringUtils(const StringUtils&);
  StringUtils& operator=(const StringUtils&);
};

#include "StringUtils.tpp"
