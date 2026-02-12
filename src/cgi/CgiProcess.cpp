/**
 * CgiProcess.cpp
 *
 * Implementation of CGI process tracking and async execution
 */

#include "CgiProcess.hpp"

#include <sys/signal.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>

CgiProcess::CgiProcess(const std::string& script_path,
                       const std::string& interpreter, int pipe_in_write,
                       int pipe_out_read, pid_t pid, int timeout_secs,
                       const std::string& request_body)
    : pid_(pid),
      script_path_(script_path),
      interpreter_(interpreter),
      pipe_in_write_(pipe_in_write),
      pipe_out_read_(pipe_out_read),
      request_body_(request_body),
      body_bytes_written_(0),
      headers_complete_(false),
      status_code_(200),
      state_(RUNNING),
      start_time_(time(NULL)),
      timeout_secs_(timeout_secs) {}

CgiProcess::~CgiProcess() {
  if (pid_ > 0) {
      kill(pid_, SIGKILL);
  }
}

bool CgiProcess::appendResponseData(const char* data, size_t len) {
  // Append to raw response
  complete_response_.append(data, len);

  // If headers are already successfully parsed, just append to body
  if (headers_complete_) {
    response_body_.append(data, len);
    return true;
  }

  return tryParseHeaders();
}

bool CgiProcess::tryParseHeaders() {
  // Look for header/body separator
  size_t sep_pos = complete_response_.find("\r\n\r\n");
  if (sep_pos == std::string::npos) {
    sep_pos = complete_response_.find("\n\n");
    if (sep_pos == std::string::npos) {
      // Separator not found yet
      return false;
    }
    // Found \n\n
    response_headers_ = complete_response_.substr(0, sep_pos);
    response_body_ = complete_response_.substr(sep_pos + 2);
  } else {
    // Found \r\n\r\n
    response_headers_ = complete_response_.substr(0, sep_pos);
    response_body_ = complete_response_.substr(sep_pos + 4);
  }

  headers_complete_ = true;

  // Parse status code from headers
  // Look for "Status: XXX" header
  std::istringstream iss(response_headers_);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty() && line[line.length() - 1] == '\r')
      line.erase(line.length() - 1);

    if (line.substr(0, 7) == "Status:") {
      int code = std::atoi(line.substr(8).c_str());
      if (code >= 100 && code < 600) {
        status_code_ = code;
      }
      // If invalid or 0, keep default 200 status
      break;
    }
  }

  return true;
}

bool CgiProcess::isTimedOut() const {
  if (state_ == RUNNING) {
    time_t now = time(NULL);
    return (now - start_time_) >= timeout_secs_;
  }
  return false;
}
