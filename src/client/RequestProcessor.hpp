#ifndef REQUEST_PROCESSOR_HPP
#define REQUEST_PROCESSOR_HPP

#include <string>
#include <vector>

#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

class LocationConfig;

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
  bool handleParseOrMethodErrors(const HttpRequest& request,
                                 int parseErrorCode,
                                 const ServerConfig* server,
                                 ProcessingResult& result) const;

  bool handleLocationValidation(const HttpRequest& request,
                                const ServerConfig* server,
                                const LocationConfig* location,
                                bool shouldClose,
                                ProcessingResult& result) const;

  bool handleCgi(const HttpRequest& request, const ServerConfig* server,
                 const LocationConfig* location,
                 const std::string& resolvedPath,
                 ProcessingResult& result) const;
};

#endif  // REQUEST_PROCESSOR_HPP
