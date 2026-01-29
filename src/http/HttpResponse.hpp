#pragma once

#include <map>
#include <string>

class HttpResponse {
public:
  HttpResponse(int status_code = 200);
  ~HttpResponse();

  void setHeader(const std::string &key, const std::string &value);
  void setBody(const std::string &body);
  void setStatus(int status_code);

  std::string toString() const;

private:
  int status_code_;
  std::string reason_phrase_;
  std::string body_;
  std::map<std::string, std::string> headers_;

  std::string getReasonPhrase(int code) const;
};
