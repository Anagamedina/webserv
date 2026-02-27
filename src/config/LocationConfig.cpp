#include "LocationConfig.hpp"

#include <algorithm>
#include <iostream>

#include "../common/namespaces.hpp"

LocationConfig::LocationConfig()
    : autoindex_(false),
      redirect_code_(-1),
      redirect_param_count_(0),
      max_body_size_(config::section::max_body_size) {}

LocationConfig::LocationConfig(const LocationConfig& other)
    : path_(other.path_),
      root_(other.root_),
      indexes_(other.indexes_),
      allowed_methods_(other.allowed_methods_),
      autoindex_(other.autoindex_),
      upload_store_(other.upload_store_),
      redirect_code_(other.redirect_code_),
      redirect_url_(other.redirect_url_),
      redirect_param_count_(other.redirect_param_count_),
      max_body_size_(other.max_body_size_),
      cgi_handlers_(other.cgi_handlers_) {}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
  if (this != &other) {
    path_ = other.path_;
    root_ = other.root_;
    indexes_ = other.indexes_;
    allowed_methods_ = other.allowed_methods_;
    autoindex_ = other.autoindex_;
    upload_store_ = other.upload_store_;
    redirect_code_ = other.redirect_code_;
    redirect_url_ = other.redirect_url_;
    redirect_param_count_ = other.redirect_param_count_;
    max_body_size_ = other.max_body_size_;
    cgi_handlers_ = other.cgi_handlers_;
  }
  return *this;
}

LocationConfig::~LocationConfig() {}

//	SETTERS
void LocationConfig::setPath(const std::string& path) { path_ = path; }
void LocationConfig::setRoot(const std::string& root) { root_ = root; }

void LocationConfig::addIndex(const std::string& index) {
  indexes_.push_back(index);
}

void LocationConfig::addMethod(const std::string& method) {
  allowed_methods_.push_back(method);
}

void LocationConfig::setAutoIndex(const bool autoindex) {
  autoindex_ = autoindex;
}

void LocationConfig::setUploadStore(const std::string& store) {
  upload_store_ = store;
}

void LocationConfig::setRedirectCode(const int integerCode) {
  redirect_code_ = integerCode;
}

void LocationConfig::setRedirectUrl(const std::string& redirectUrl) {
  redirect_url_ = redirectUrl;
}

void LocationConfig::setRedirectParamCount(const int count) {
  redirect_param_count_ = count;
}

void LocationConfig::setMaxBodySize(size_t size) { max_body_size_ = size; }

//	GETTERS
void LocationConfig::addCgiHandler(const std::string& extension,
                                   const std::string& binaryPath) {
  cgi_handlers_.insert(
      std::pair<std::string, std::string>(extension, binaryPath));
}

const std::string& LocationConfig::getPath() const { return path_; }
const std::string& LocationConfig::getRoot() const { return root_; }

const std::vector<std::string>& LocationConfig::getIndexes() const {
  return indexes_;
}

const std::vector<std::string>& LocationConfig::getMethods() const {
  return allowed_methods_;
}

bool LocationConfig::getAutoIndex() const { return autoindex_; }

const std::string& LocationConfig::getUploadStore() const {
  return upload_store_;
}

int LocationConfig::getRedirectCode() const { return redirect_code_; }

const std::string& LocationConfig::getRedirectUrl() const {
  return redirect_url_;
}

int LocationConfig::getRedirectParamCount() const {
  return redirect_param_count_;
}

size_t LocationConfig::getMaxBodySize() const { return max_body_size_; }

std::string LocationConfig::getCgiPath(const std::string& extension) const {
  const std::map<std::string, std::string>::const_iterator it =
      cgi_handlers_.find(extension);

  if (it != cgi_handlers_.end()) {
    return it->second;
  }
  return "";
}

const std::map<std::string, std::string>& LocationConfig::getCgiHandlers()
    const {
  return cgi_handlers_;
}

/**
 * HEAD must be explicitly allowed in config (not implicitly via GET)
 * Each method is independent and must be listed in allowed_methods
 * @param method
 * @return true or false
 */
bool LocationConfig::isMethodAllowed(const std::string& method) const {
  if (method.empty()) {
    return false;
  }
  // if not set method, default method is GET and HEAD
  if (allowed_methods_.empty()) {
    return (method == config::section::method_get);
    // return (method == config::section::method_get ||
    // method == config::section::method_head);
  }
  if (std::find(allowed_methods_.begin(), allowed_methods_.end(), method) !=
      allowed_methods_.end()) {
    return true;
  }
  return false;
}

void LocationConfig::print() const { std::cout << *this << std::endl; }

/**
 * @brief Formats and outputs the LocationConfig information to a stream with ANSI colors.
 *
 * @param os The output stream.
 * @param location The LocationConfig object to print.
 * @return std::ostream& Reference to the modified output stream.
 */
std::ostream& operator<<(std::ostream& os, const LocationConfig& location) {
  os << config::colors::cyan << config::colors::bold << "Locations info:\n"
     << config::colors::reset << "\t" << config::colors::yellow
     << "Location Path: " << config::colors::reset << config::colors::green
     << location.getPath() << config::colors::reset << "\n"
     << "\t" << config::colors::yellow << "Root: " << config::colors::reset
     << config::colors::green << location.getRoot() << config::colors::reset
     << "\n"
     << "\t" << config::colors::yellow
     << "Autoindex: " << config::colors::reset;
  if (location.getAutoIndex()) {
    os << config::colors::green << "on";
  } else {
    os << config::colors::red << "off";
  }
  os << config::colors::reset << "\n";

  if (!location.getUploadStore().empty()) {
    os << "\t" << config::colors::yellow
       << "Upload Store: " << config::colors::reset << config::colors::green
       << location.getUploadStore() << config::colors::reset << "\n";
  } else {
    os << "\t" << config::colors::yellow << "empty" << config::colors::reset
       << "\n";
  }

  os << "\t" << config::colors::yellow
     << "Max Body Size: " << config::colors::reset << config::colors::green
     << location.getMaxBodySize() << config::colors::reset << "\n";

  if (location.getRedirectCode() != -1) {
    os << "\t" << config::colors::yellow << "Return ("
       << location.getRedirectParamCount() << " parameter"
       << (location.getRedirectParamCount() == 1 ? "" : "s")
       << "): " << config::colors::reset;
    if (location.getRedirectParamCount() == 1) {
      os << config::colors::green << location.getRedirectUrl()
         << config::colors::reset << "\n";
    } else {
      os << config::colors::green << location.getRedirectCode() << " '"
         << location.getRedirectUrl() << "'" << config::colors::reset << "\n";
    }
  }

  const std::vector<std::string>& indexes = location.getIndexes();
  os << "\t" << config::colors::yellow << "Indexes: " << config::colors::reset;
  if (indexes.empty()) {
    os << config::colors::red << "empty" << config::colors::reset;
  } else {
    for (size_t i = 0; i < indexes.size(); ++i) {
      os << config::colors::green << indexes[i] << config::colors::reset
         << (i == indexes.size() - 1 ? "" : ", ");
    }
  }

  // Print Methods
  const std::vector<std::string>& methods = location.getMethods();
  os << "\n\t" << config::colors::yellow
     << "Methods: " << config::colors::reset;
  if (methods.empty()) {
    os << config::colors::red << "empty" << config::colors::reset;
  } else {
    for (size_t i = 0; i < methods.size(); ++i) {
      os << config::colors::green << methods[i] << config::colors::reset
         << (i == methods.size() - 1 ? "" : ", ");
    }
  }
  os << "\n";
  // -------------------------------------------------
  //           BONUS: CGI Handlers
  // -------------------------------------------------
  os << "\n\t" << config::colors::magenta << config::colors::bold
     << "CGI Handlers (bonus):" << config::colors::reset << "\n";

  const std::map<std::string, std::string>& cgi = location.getCgiHandlers();

  if (cgi.empty()) {
    os << "\t" << config::colors::yellow << "  CGI: " << config::colors::reset
       << config::colors::red << "none configured" << config::colors::reset
       << "\n";
  } else {
    std::map<std::string, std::string>::const_iterator it = cgi.begin();
    for (; it != cgi.end(); ++it) {
      os << "\t" << config::colors::yellow << "[" << it->first
         << "]: " << config::colors::reset << config::colors::green
         << it->second << config::colors::reset << "\n";
    }
  }

  return os;
}
