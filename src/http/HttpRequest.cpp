#include "HttpRequest.hpp"

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

void HttpRequest::setMethod(const std::string& method) { method_ = method; }
void HttpRequest::setUri(const std::string& uri) { uri_ = uri; }
void HttpRequest::setVersion(const std::string& version) { version_ = version; }

void HttpRequest::setHeader(const std::string& key, const std::string& value) {
  headers_[key] = value;
}

void HttpRequest::setBody(const std::string& body) { body_ = body; }

void HttpRequest::appendBody(const std::string& data) { body_ += data; }

std::string HttpRequest::getMethod() const { return method_; }
std::string HttpRequest::getUri() const { return uri_; }
std::string HttpRequest::getVersion() const { return version_; }

std::string HttpRequest::getHeader(const std::string& key) const {
  std::map<std::string, std::string>::const_iterator it = headers_.find(key);
  if (it != headers_.end()) {
    return it->second;
  }
  return "";
}

std::string HttpRequest::getBody() const { return body_; }

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
  return headers_;
}

void HttpRequest::clear() {
  method_.clear();
  uri_.clear();
  version_.clear();
  headers_.clear();
  body_.clear();
}

void HttpRequest::print() const {
  std::cout << "Method: " << method_ << std::endl;
  std::cout << "URI: " << uri_ << std::endl;
  std::cout << "Version: " << version_ << std::endl;
  std::cout << "Headers:" << std::endl;
  for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
       it != headers_.end(); ++it) {
    std::cout << "  " << it->first << ": " << it->second << std::endl;
  }
  std::cout << "Body Size: " << body_.size() << std::endl;
}
