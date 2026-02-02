#ifndef WEBSERV_SERVERCONFIG_HPP
#define WEBSERV_SERVERCONFIG_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "LocationConfig.hpp"
#include "../common/namespaces.hpp"

/**
 * ServerConfig stores configuration for one server { } block
 *
 * Corresponds to nginx-like config:
 * server {
 *     listen 8080;
 *     host 127.0.0.1;
 *     server_name example.com;
 *     max_body_size 1048576;
 *     error_page 404 /404.html;
 *     location / { ... }
 * }
 */
class ServerConfig
{
private:
	int listen_port_; // listen 8080
	std::string host_address_; // host 127.0.0.1
	std::string server_name_; // (optional ??)
	std::string root_; //	root server
	std::string index_; //	root server
	size_t max_body_size_; // max is 1048576 (bytes)
	std::map<int, std::string> error_pages_;
	// Error pages: error_page 404 /404.html
	std::vector<LocationConfig> location_; // All location { } blocks

public:
	typedef std::map<int, std::string> ErrorMap;
	typedef ErrorMap::const_iterator ErrorIterator;

	ServerConfig();
	ServerConfig(const ServerConfig& other);
	ServerConfig& operator=(const ServerConfig& other);
	~ServerConfig();

	// Validation
	// bool isValid() const;

	// Setters
	void setPort(int port);
	void setHost(const std::string& host);
	void setServerName(const std::string& name);
	void setRoot(const std::string& root);
	void setIndex(const std::string& index);
	void setMaxBodySize(size_t size);
	void addErrorPage(int code, const std::string& path);
	void addLocation(const LocationConfig& location);

	// Getters
	int getPort() const;
	const std::string& getHost() const;
	const std::string& getServerName() const;
	const std::string& getRoot() const;
	const std::string& getIndex() const;
	size_t getMaxBodySize() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::vector<LocationConfig>& getLocations() const;

	// Debug
	void print() const;
};

inline std::ostream& operator<<(std::ostream& os, const ServerConfig& config)
{
	os << config::colors::blue << "Server Config:\n" << config::colors::reset
		<< "\tPort: " << config.getPort()
		<< "\n\tHost: " << config.getHost()
		<< "\n\tServer name: " << config.getServerName() << "\n";

	const ServerConfig::ErrorMap& errorPages = config.getErrorPages();
	os << "Error pages:\n";

	if (!errorPages.empty())
	{
		std::map<std::string, std::vector<int>> groupedErrors;

		for (ServerConfig::ErrorIterator it = errorPages.begin(); it !=
			errorPages.end(); ++it)
		{
			groupedErrors[it->second].push_back(it->first);
		}

		for (std::map<std::string, std::vector<int>>::const_iterator groupIt =
				groupedErrors.begin(); groupIt != groupedErrors.end();
			++groupIt)
		{
			os << "\t";
			const std::vector<int>& codes = groupIt->second;
			for (std::vector<int>::const_iterator codeIt = codes.begin(); codeIt
				!= codes.end(); ++codeIt)
			{
				os << " " << *codeIt;
			}
			os << " " << groupIt->first << "\n";
		}
	}
	else
	{
		os << "\tNot configured\n";
	}

	const std::vector<LocationConfig>& locations = config.getLocations();
	for (size_t i = 0; i < locations.size(); ++i)
	{
		os << locations[i];
	}
	return os;
}

#endif // WEBSERV_SERVERCONFIG_HPP
