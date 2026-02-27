#include "ServerConfig.hpp"

#include <sstream>
#include <string>

#include "../common/namespaces.hpp"
#include "ConfigException.hpp"
#include "LocationConfig.hpp"

ServerConfig::ServerConfig()
    : listen_port_(config::section::default_port),
      max_body_size_(config::section::max_body_size),
      cgi_timeout_(60),
      autoindex_(false),
      redirect_code_(-1) {}

ServerConfig::ServerConfig(const ServerConfig& other)
    : listen_port_(other.listen_port_),
      host_address_(other.host_address_),
      server_name_(other.server_name_),
      root_(other.root_),
      indexes_(other.indexes_),
      max_body_size_(other.max_body_size_),
      cgi_timeout_(other.cgi_timeout_),
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
    cgi_timeout_ = other.cgi_timeout_;
    error_pages_ = other.error_pages_;
    locations_ = other.locations_;
    autoindex_ = other.autoindex_;
    redirect_code_ = other.redirect_code_;
    redirect_url_ = other.redirect_url_;
  }
  return *this;
}

ServerConfig::~ServerConfig() {}

//	SETTERS
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

void ServerConfig::setCgiTimeout(int seconds) {
  if (seconds < 1) {
    throw ConfigException("Invalid CGI timeout: must be >= 1 second");
  }
  cgi_timeout_ = seconds;
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
  if (code < 100 || code > 599) {
    std::stringstream ss;
    ss << code;
    throw ConfigException(config::errors::invalid_http_status_code + ss.str());
  }
  error_pages_[code] = path;
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

int ServerConfig::getCgiTimeout() const { return cgi_timeout_; }

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

size_t ServerConfig::getGlobalMaxBodySize() const {
  size_t max = max_body_size_;
  if (max == 0) return 0;  // Unlimited

  for (size_t i = 0; i < locations_.size(); ++i) {
    size_t locMax = locations_[i].getMaxBodySize();
    if (locMax == 0) return 0;  // Unlimited in a location -> Global unlimited
    if (locMax > max) {
      max = locMax;
    }
  }
  return max;
}

void ServerConfig::print() const { std::cout << *this; }

/**
 * @brief Formats and outputs the ServerConfig information to a stream with ANSI colors.
 *
 * @param os The output stream.
 * @param config The ServerConfig object to print.
 * @return std::ostream& Reference to the modified output stream.
 */
std::ostream& operator<<(std::ostream& os, const ServerConfig& config) {
  os << config::colors::blue << config::colors::bold << "Server Config:\n"
     << config::colors::reset << "\t" << config::colors::yellow
     << "Port: " << config::colors::reset << config::colors::green
     << config.getPort() << config::colors::reset << "\n"
     << "\t" << config::colors::yellow << "Host: " << config::colors::reset
     << config::colors::green << config.getHost() << config::colors::reset
     << "\n"
     << "\t" << config::colors::yellow
     << "Server name: " << config::colors::reset << config::colors::green
     << config.getServerName() << config::colors::reset << "\n"
     << config::colors::yellow << "\tMax body size: " << config::colors::reset
     << config::colors::green << config.getMaxBodySize()
     << config::colors::reset << "\n";

  const ServerConfig::ErrorMap& errorPages = config.getErrorPages();
  os << "\t" << config::colors::yellow << "Error pages:\n"
     << config::colors::reset;
  if (!errorPages.empty()) {
    std::map<std::string, std::vector<int> > groupedErrors;
    for (ServerConfig::ErrorIterator it = errorPages.begin();
         it != errorPages.end(); ++it) {
      groupedErrors[it->second].push_back(it->first);
    }
    for (std::map<std::string, std::vector<int> >::const_iterator groupIt =
             groupedErrors.begin();
         groupIt != groupedErrors.end(); ++groupIt) {
      os << "\t";
      const std::vector<int>& codes = groupIt->second;
      for (std::vector<int>::const_iterator codeIt = codes.begin();
           codeIt != codes.end(); ++codeIt) {
        os << config::colors::magenta << " " << *codeIt
           << config::colors::reset;
      }
      os << config::colors::green << " " << groupIt->first
         << config::colors::reset << "\n";
    }
  } else {
    os << config::colors::red << "\t\tNot configured" << config::colors::reset
       << "\n";
  }

  const std::vector<LocationConfig>& locations = config.getLocations();
  for (size_t i = 0; i < locations.size(); ++i) {
    os << locations[i];
  }
  return os;
}
