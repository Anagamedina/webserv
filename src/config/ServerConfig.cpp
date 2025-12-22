#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <iostream>

// Default values
static const int DEFAULT_PORT = 8080;
static const std::string DEFAULT_HOST = "127.0.0.1";
static const size_t DEFAULT_MAX_BODY_SIZE = 1048576; // 1 MB

ServerConfig::ServerConfig() : listen_port_(DEFAULT_PORT), host_address_(DEFAULT_HOST) ,max_body_size_(DEFAULT_MAX_BODY_SIZE)
{
}

ServerConfig::~ServerConfig()
{
}
