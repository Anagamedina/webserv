#pragma once

#include <iostream>
#include <map>
#include <string>

class HttpRequest {
public:
  HttpRequest();
  ~HttpRequest();

  void setMethod(const std::string &method);
  void setUri(const std::string &uri);
  void setVersion(const std::string &version);
  void setHeader(const std::string &key, const std::string &value);
  void setBody(const std::string &body);
  void appendBody(const std::string &data);

  std::string getMethod() const;
  std::string getUri() const;
  std::string getVersion() const;
  std::string getHeader(const std::string &key) const;
  std::string getBody() const;
  const std::map<std::string, std::string> &getHeaders() const;

  void clear();

  // Debugging
  void print() const;

private:
  std::string method_;
  std::string uri_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};
