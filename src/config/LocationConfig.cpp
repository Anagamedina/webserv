#include "LocationConfig.hpp"

#include <iostream>

LocationConfig::LocationConfig()
    : autoindex_(false), redirect_code_(-1), redirect_param_count_(0) {}

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
    cgi_handlers_ = other.cgi_handlers_;
  }
  return *this;
}

LocationConfig::~LocationConfig() {}

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
 * this function are doing two actions is possible we need to refactor the
 * impplementation ?
 * @param method
 * @return true or false
 */
bool LocationConfig::isMethodAllowed(const std::string& method) const {
  if (method.empty()) {
    return false;
  }
  // Per HTTP spec (RFC 2616 §5.1.1 / RFC 9110 §9.3.2):
  // HEAD is identical to GET except the server MUST NOT return a body.
  // A server that supports GET MUST also support HEAD.
  // So we treat HEAD as GET for method-validation purposes.
  std::string effectiveMethod = method;
  if (effectiveMethod == config::section::method_head) {
    effectiveMethod = config::section::method_get;
  }
  if (allowed_methods_.empty()) {
    return (effectiveMethod == config::section::method_get);
  }

  for (size_t i = 0; i < allowed_methods_.size(); ++i) {
    if (allowed_methods_[i] == effectiveMethod) return true;
  }
  return false;
}

void LocationConfig::print() const { std::cout << *this << std::endl; }
