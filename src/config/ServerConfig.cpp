#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <iostream>

#include "ConfigException.hpp"

ServerConfig::ServerConfig() :
	listen_port_(80),
	host_address_("127.0.0.1"),
	max_body_size_(1000000)
{
}

ServerConfig::ServerConfig(const ServerConfig& other) :
	listen_port_(other.listen_port_),
	host_address_(other.host_address_),
	server_name_(other.server_name_),
	root_(other.root_),
	index_(other.index_),
	max_body_size_(other.max_body_size_),
	error_pages_(other.error_pages_),
	location_(other.location_)
{
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other)
{
	if (this != &other)
	{
		listen_port_ = other.listen_port_;
		host_address_ = other.host_address_;
		root_ = other.root_;
		index_ = other.index_;
		server_name_ = other.server_name_;
		max_body_size_ = other.max_body_size_;
		error_pages_ = other.error_pages_;
	}
	return *this;
}

ServerConfig::~ServerConfig()
{
}

//	GETTERS AND SETTERS
void ServerConfig::setPort(int port)
{
	if (port < 1 || port > config::section::max_port)
	{
		throw ConfigException(config::errors::invalid_port_range);
	}
	listen_port_ = port;
}

void ServerConfig::setHost(const std::string& host)
{
	host_address_ = host;
}

void ServerConfig::setServerName(const std::string& name)
{
	server_name_ = name;
}

void ServerConfig::setRoot(const std::string& root)
{
	root_ = root;
}

void ServerConfig::setIndex(const std::string& index)
{
	index_ = index;
}

void ServerConfig::setMaxBodySize(size_t size)
{
	max_body_size_ = size;
}

void ServerConfig::addErrorPage(int code, const std::string& path)
{
	if (code < 100 || code > 599)
	{
		throw ConfigException(config::errors::invalid_http_status_code);
	}
	error_pages_.insert(std::make_pair(code, path));
}

void ServerConfig::addLocation(const LocationConfig& location)
{
	location_.push_back(location);
}

//	GETTERS

int ServerConfig::getPort() const
{
	return listen_port_;
}

const std::string& ServerConfig::getHost() const
{
	return host_address_;
}

const std::string& ServerConfig::getServerName() const
{
	return server_name_;
}

const std::string& ServerConfig::getRoot() const
{
	return root_;
}

const std::string& ServerConfig::getIndex() const
{
	return index_;
}

size_t ServerConfig::getMaxBodySize() const
{
	return max_body_size_;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const
{
	return error_pages_;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const
{
	return location_;
}

void ServerConfig::print() const
{
	std::cout << "Print of info in ServerConfig";
}
