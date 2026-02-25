#ifndef WEBSERV_SERVERCONFIG_HPP
#define WEBSERV_SERVERCONFIG_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "LocationConfig.hpp"
#include "common/namespaces.hpp"

/**
 * @brief Configuration of one server { } block
 * Represents a virtual server (nginx-like style).
 *
 * Example equivalent configuration:
 * ```
 * server {
 *     listen          8080;
 *     host            127.0.0.1;
 *     server_name     example.com;
 *     root            /var/www/html;
 *     index           index.html index.htm;
 *     max_body_size   1048576;
 *     error_page      404 /errors/404.html;
 *     autoindex       on;
 *     return          301 http://example.com;
 *     location / { ... }
 * }
 * ```
 */
class ServerConfig {
 public:
  typedef std::map<int, std::string> ErrorMap;
  typedef ErrorMap::const_iterator ErrorIterator;

  ServerConfig();
  ServerConfig(const ServerConfig& other);
  ServerConfig& operator=(const ServerConfig& other);
  ~ServerConfig();

  // Setters
  void setPort(int port);
  void setHost(const std::string& host);
  void setServerName(const std::string& name);
  void setRoot(const std::string& root);
  void addIndex(const std::string& index);
  void setMaxBodySize(size_t size);
  void setCgiTimeout(int seconds);
  void addErrorPage(int code, const std::string& path);
  void addLocation(const LocationConfig& location);
  void setAutoIndex(bool autoindex);
  void setRedirectCode(int code);
  void setRedirectUrl(const std::string& url);

  // Getters
  int getPort() const;
  const std::string& getHost() const;
  const std::string& getServerName() const;
  const std::string& getRoot() const;
  const std::vector<std::string>& getIndexVector() const;
  size_t getMaxBodySize() const;
  int getCgiTimeout() const;
  const std::map<int, std::string>& getErrorPages() const;
  const std::vector<LocationConfig>& getLocations() const;
  bool getAutoindex() const;
  int getRedirectCode() const;
  const std::string& getRedirectUrl() const;
  size_t getGlobalMaxBodySize() const;

  // Debug Helper
  void print() const;

 private:
  int listen_port_;
  std::string host_address_;
  std::string server_name_;
  std::string root_;
  std::vector<std::string> indexes_;
  size_t max_body_size_;
  int cgi_timeout_;
  std::map<int, std::string> error_pages_;
  std::vector<LocationConfig> locations_;
  bool autoindex_;
  int redirect_code_;
  std::string redirect_url_;
};

std::ostream& operator<<(std::ostream& os, const ServerConfig& config);

#endif  // WEBSERV_SERVERCONFIG_HPP
