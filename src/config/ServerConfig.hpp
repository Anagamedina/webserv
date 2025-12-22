#ifndef WEBSERV_SERVERCONFIG_HPP
#define WEBSERV_SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>

class LocationConfig;

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
	int listen_port_;	// listen 8080
	std::string host_address_;	// host 127.0.0.1
	std::string server_name_;	// (optional ??)
	size_t max_body_size_;                         // max is 1048576 (bytes)
	std::map<int, std::string> error_pages_;       // Map: HTTP code -> file path | Error pages: error_page 404 /404.html
	std::vector<LocationConfig> location_configs_; // All location { } blocks

public:
	ServerConfig();
	ServerConfig(const ServerConfig& other);
	ServerConfig& operator=(const ServerConfig& other);
	~ServerConfig();
	
	// Getters
	int getPort() const;
	const std::string& getHost() const;
	const std::string& getServerName() const;
	size_t getMaxBodySize() const;
	const std::map<int, std::string>& getErrorPages() const;
	const std::vector<LocationConfig>& getLocations() const;
	
	// Setters
	void setPort(int port);
	void setHost(const std::string& host);
	void setServerName(const std::string& name);
	void setMaxBodySize(size_t size);
	void addErrorPage(int code, const std::string& path);
	void addLocation(const LocationConfig& location);
	
	// Validation
	bool isValid() const;
	
	// Debug
	void print() const;
};

#endif //WEBSERV_SERVERCONFIG_HPP