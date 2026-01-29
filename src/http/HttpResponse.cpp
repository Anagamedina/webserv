#include "HttpResponse.hpp"
#include "../utils/StringUtils.hpp"
#include <sstream>

HttpResponse::HttpResponse(int status_code) : status_code_(status_code) {
  reason_phrase_ = getReasonPhrase(status_code);
}

HttpResponse::~HttpResponse() {}

void HttpResponse::setHeader(const std::string &key, const std::string &value) {
  headers_[key] = value;
}

void HttpResponse::setBody(const std::string &body) {
  body_ = body;
  // Auto-set Content-Length if not set manually?
  // Better to let caller control or auto-calculate in toString
}

void HttpResponse::setStatus(int status_code) {
  status_code_ = status_code;
  reason_phrase_ = getReasonPhrase(status_code);
}

std::string HttpResponse::toString() const {
  std::ostringstream response;

  response << "HTTP/1.1 " << status_code_ << " " << reason_phrase_ << "\r\n";

  // Auto Headers: Content-Length, Server, Date
  // Note: We should probably check if they are already set
  // For now, let's append headers
  for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
       it != headers_.end(); ++it) {
    response << it->first << ": " << it->second << "\r\n";
  }

  // Mandatory Headers Check
  if (headers_.count("Content-Length") == 0) {
    response << "Content-Length: " << body_.length() << "\r\n";
  }
  if (headers_.count("Connection") == 0) {
    response << "Connection: close\r\n";
  }

  response << "\r\n";
  response << body_;

  return response.str();
}

std::string HttpResponse::getReasonPhrase(int code) const {
  switch (code) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 413:
    return "Payload Too Large";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 505:
    return "HTTP Version Not Supported";
  default:
    return "Unknown Status";
  }
}
