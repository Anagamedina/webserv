#ifndef REQUEST_PROCESSOR_HPP
#define REQUEST_PROCESSOR_HPP

#include <string>
#include <vector>

#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

class RequestProcessor {
 public:
  enum ActionType { ACTION_SEND_RESPONSE, ACTION_EXECUTE_CGI };

  struct CgiInfo {
    CgiInfo() : server(0) {}
    std::string scriptPath;
    std::string interpreterPath;
    const ServerConfig* server;
  };

  struct ProcessingResult {
    ProcessingResult() : action(ACTION_SEND_RESPONSE) {}
    ActionType action;
    HttpResponse response;  // For static/error
    CgiInfo cgiInfo;        // For CGI
  };

  ProcessingResult process(const HttpRequest& request,
                           const std::vector<ServerConfig>* configs,
                           int listenPort, int parseErrorCode);

 private:
};

#endif  // REQUEST_PROCESSOR_HPP
