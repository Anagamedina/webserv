#include "ConfigException.hpp"
#include <iostream>

ConfigException::ConfigException(const std::string& message) 
	: message_("Config Error: " + message) {}

ConfigException::~ConfigException() throw() {}

const char* ConfigException::what() const throw() {
	return message_.c_str();
}

// Private/Disallowed methods implementation (Optional, but good for completeness)
// To prevent linker errors if these are accidentally used
ConfigException::ConfigException() {}
ConfigException& ConfigException::operator=(const ConfigException& other) {
    (void)other;
    return *this;
}
ConfigException::ConfigException(const ConfigException& other) 
    : message_(other.message_) {}
