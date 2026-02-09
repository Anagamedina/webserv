#include "ConfigException.hpp"

ConfigException::ConfigException(const std::string& msg) :
      message_(msg) {
}

ConfigException::~ConfigException() throw() {
}

const char* ConfigException::what() const throw() {
    return message_.c_str();
}
