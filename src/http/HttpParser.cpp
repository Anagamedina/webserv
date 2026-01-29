#include "HttpParser.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>

HttpParser::HttpParser() : state_(REQUEST_LINE), content_length_(0) {}

HttpParser::~HttpParser() {}

void HttpParser::reset() {
  request_.clear();
  state_ = REQUEST_LINE;
  buffer_.clear();
  content_length_ = 0;
}

void HttpParser::parse(const char* data, size_t size) {
  if (state_ == COMPLETE || state_ == ERROR)
    return;

  buffer_.append(data, size);

  // Process buffer line by line
  while (state_ != COMPLETE && state_ != ERROR && state_ != BODY) {
    size_t pos = buffer_.find("\r\n");
    if (pos == std::string::npos) {
      break; // Wait for more data
    }

    std::string line = buffer_.substr(0, pos);
    buffer_.erase(0, pos + 2); // Remove line + \r\n

    if (state_ == REQUEST_LINE) {
      if (line.empty())
        continue; // Skip empty lines before request
      try {
        parseRequestLine(line);
        state_ = HEADERS;
      } catch (...) {
        state_ = ERROR;
      }
    } else if (state_ == HEADERS) {
      if (line.empty()) {
        // End of headers
        checkHeadersForBody();
      } else {
        parseHeader(line);
      }
    }
  }

  if (state_ == BODY) {
    // Read body
    if (content_length_ > 0) {
      size_t needed = content_length_ - request_.getBody().size();
      size_t available = buffer_.size();
      size_t to_take = (available < needed) ? available : needed;

      request_.appendBody(buffer_.substr(0, to_take));
      buffer_.erase(0, to_take);

      if (request_.getBody().size() >= content_length_) {
        state_ = COMPLETE;
      }
    } else {
      // No content length, assume no body (for now, chunked not supported)
      state_ = COMPLETE;
    }
  }
}

void HttpParser::parseRequestLine(std::string& line) {
  std::istringstream iss(line);
  std::string method, uri, version;
  if (!(iss >> method >> uri >> version)) {
    throw std::runtime_error("Invalid request line");
  }
  request_.setMethod(method);
  request_.setUri(uri);
  request_.setVersion(version);
}

void HttpParser::parseHeader(std::string& line) {
  size_t pos = line.find(':');
  if (pos != std::string::npos) {
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    // Normalize key to lowercase
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    // Trim whitespace
    size_t first = value.find_first_not_of(" \t");
    if (first != std::string::npos)
      value = value.substr(first);
    size_t last = value.find_last_not_of(" \t");
    if (last != std::string::npos)
      value = value.substr(0, last + 1);

    request_.setHeader(key, value);
  }
}

void HttpParser::checkHeadersForBody() {
  std::string cl =
      request_.getHeader("content-length"); // Keys are lowercased now
  if (!cl.empty()) {
    int len = std::atoi(cl.c_str());
    if (len < 0) {
      state_ = ERROR;
      return;
    }
    content_length_ = static_cast<size_t>(len);
    state_ = BODY;
    // If there's already data in buffer for body, the main loop will catch it
    // in next iteration? No, main loop condition `while != BODY`. So we need to
    // let it fall through or re-check. Actually, my main loop sends us to BODY
    // handling immediately after this function returns if I structured it
    // right? Ah, the while loop condition is `state_ != BODY`. So if we switch
    // to BODY inside checks, the loop terminates and we go to `if (state_ ==
    // BODY)` block. Correct.
  } else {
    state_ = COMPLETE;
  }
}

bool HttpParser::isComplete() const { return state_ == COMPLETE; }

const HttpRequest& HttpParser::getRequest() const { return request_; }

HttpParser::State HttpParser::getState() const { return state_; }
