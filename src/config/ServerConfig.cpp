#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <iostream>

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

std::ostream& operator<<(std::ostream& os, const ServerConfig& config)
{
	os << "Server Config:" << "\n"
		<< "  Port: " << config.listen_port_ << "\n"
		<< "  Host: " << config.host_address_ << "\n"
		<< "  Name: " << config.server_name_ << "\n";

	std::map<int, std::string>::const_iterator it;

	for (it = config.error_pages_.begin(); it != config.error_pages_.end(); ++it) {
		os << "  Error " << it->first << ": " << it->second << "\n";
	}
	return os;
}

void ServerConfig::print() const
{
	std::cout << "Print of info in ServerConfig";
}
