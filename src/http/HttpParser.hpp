#pragma once

#include "HttpRequest.hpp"
#include <string>

class HttpParser {
public:
  enum State {
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
  };

  HttpParser();
  ~HttpParser();

  void parse(const char* data, size_t size);
  bool isComplete() const;
  const HttpRequest& getRequest() const;
  State getState() const;
  void reset();

private:
  HttpRequest request_;
  State state_;
  std::string buffer_;
  size_t content_length_;

  // Parsing helpers
  void parseRequestLine(std::string& line);
  void parseHeader(std::string& line);
  void checkHeadersForBody();
};
