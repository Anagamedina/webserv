#ifndef WEBSERV_SERVERCONFIG_HPP
#define WEBSERV_SERVERCONFIG_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

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
class ServerConfig {
private:
  int listen_port_;          // listen 8080
  std::string host_address_; // host 127.0.0.1
  std::string server_name_;  // (optional ??)
  std::string root_;         //	root server
  std::string index_;        //	root server
  size_t max_body_size_;     // max is 1048576 (bytes)
  std::map<int, std::string> error_pages_;
  // Error pages: error_page 404 /404.html
  std::vector<LocationConfig> location_; // All location { } blocks

public:
  ServerConfig();
  ServerConfig(const ServerConfig &other);
  ServerConfig &operator=(const ServerConfig &other);
  ~ServerConfig();

  // Validation
  // bool isValid() const;

  // Setters
  void setPort(int port);
  void setHost(const std::string &host);
  void setServerName(const std::string &name);
  void setRoot(const std::string &root);
  void setIndex(const std::string &index);
  void setMaxBodySize(size_t size);
  void addErrorPage(int code, const std::string &path);
  void addLocation(const LocationConfig &location);

  // Getters
  int getPort() const;
  const std::string &getHost() const;
  const std::string &getServerName() const;
  const std::string &getRoot() const;
  const std::string &getIndex() const;
  size_t getMaxBodySize() const;
  const std::map<int, std::string> &getErrorPages() const;
  const std::vector<LocationConfig> &getLocations() const;

  // Debug
  void print() const;
};

// Friend function for logging/printing (Hidden Friend)
// Friend function for logging/printing
// Defined inline to avoid multiple definition errors in header
inline std::ostream &operator<<(std::ostream &os, const ServerConfig &config) {
  os << "Server Config:" << "\n"
     << "  Port: " << config.getPort() << "\n"
     << "  Host: " << config.getHost() << "\n"
     << "  Name: " << config.getServerName() << "\n";

  std::map<int, std::string>::const_iterator it;

  // config.getErrorPages() returns a const reference to map
  for (it = config.getErrorPages().begin(); it != config.getErrorPages().end();
       ++it) {
    os << "  Error " << it->first << ": " << it->second << "\n";
  }
  return os;
}
#endif // WEBSERV_SERVERCONFIG_HPP
