#include "ServerConfig.hpp"

#include "ConfigException.hpp"
#include "LocationConfig.hpp"

ServerConfig::ServerConfig()
    : listen_port_(config::section::default_port),
      host_address_(config::section::default_host_name),
      max_body_size_(config::section::max_body_size),
      autoindex_(false),
      redirect_code_(-1) {}

ServerConfig::ServerConfig(const ServerConfig& other)
    : listen_port_(other.listen_port_),
      host_address_(other.host_address_),
      server_name_(other.server_name_),
      root_(other.root_),
      indexes_(other.indexes_),
      max_body_size_(other.max_body_size_),
      error_pages_(other.error_pages_),
      locations_(other.locations_),
      autoindex_(other.autoindex_),
      redirect_code_(other.redirect_code_),
      redirect_url_(other.redirect_url_) {}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
  if (this != &other) {
    listen_port_ = other.listen_port_;
    host_address_ = other.host_address_;
    root_ = other.root_;
    indexes_ = other.indexes_;
    server_name_ = other.server_name_;
    max_body_size_ = other.max_body_size_;
    error_pages_ = other.error_pages_;
    locations_ = other.locations_;
    autoindex_ = other.autoindex_;
    redirect_code_ = other.redirect_code_;
    redirect_url_ = other.redirect_url_;
  }
  return *this;
}

ServerConfig::~ServerConfig() {}

//	GETTERS AND SETTERS
void ServerConfig::setPort(int port) {
  if (port < 1 || port > config::section::max_port) {
    throw ConfigException(config::errors::invalid_port_range);
  }
  listen_port_ = port;
}

void ServerConfig::setHost(const std::string& host) { host_address_ = host; }

void ServerConfig::setServerName(const std::string& name) {
  server_name_ = name;
}

void ServerConfig::setRoot(const std::string& root) { root_ = root; }

void ServerConfig::addIndex(const std::string& index) {
  indexes_.push_back(index);
}

void ServerConfig::setMaxBodySize(size_t size) { max_body_size_ = size; }

void ServerConfig::addErrorPage(int code, const std::string& path) {
  if (code < 100 || code > 599) {
    throw ConfigException(config::errors::invalid_http_status_code);
  }
  error_pages_.insert(std::make_pair(code, path));
}

void ServerConfig::addLocation(const LocationConfig& location) {
  locations_.push_back(location);
}

void ServerConfig::setAutoIndex(bool autoindex) { autoindex_ = autoindex; }

void ServerConfig::setRedirectCode(int code) {
  if (code < 100 || code > 599)
    throw ConfigException(config::errors::invalid_http_status_code);
  redirect_code_ = code;
}

void ServerConfig::setRedirectUrl(const std::string& url) {
  redirect_url_ = url;
}

//	GETTERS

int ServerConfig::getPort() const { return listen_port_; }

const std::string& ServerConfig::getHost() const { return host_address_; }

const std::string& ServerConfig::getServerName() const { return server_name_; }

const std::string& ServerConfig::getRoot() const { return root_; }

const std::vector<std::string>& ServerConfig::getIndexVector() const {
  return indexes_;
}

size_t ServerConfig::getMaxBodySize() const { return max_body_size_; }

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
  return error_pages_;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
  return locations_;
}

bool ServerConfig::getAutoindex() const { return autoindex_; }

int ServerConfig::getRedirectCode() const { return redirect_code_; }

const std::string& ServerConfig::getRedirectUrl() const {
  return redirect_url_;
}

void ServerConfig::print() const { std::cout << *this; }
