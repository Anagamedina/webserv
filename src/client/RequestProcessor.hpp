#ifndef REQUEST_PROCESSOR_HPP
#define REQUEST_PROCESSOR_HPP

#include <string>
#include <vector>

#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

// El cerebro del servidor: es quien decide que hacer con la peticion dado un
// HttpRequest y la configuración, construye un HttpResponse (estático, CGI,
// error, etc.) y lo devuelve al cliente. solo transforma un HttpRequest en
// HttpResponse sin enviarlo al cliente.
class RequestProcessor {
 public:
  enum ActionType { ACTION_SEND_RESPONSE, ACTION_EXECUTE_CGI };

  struct CgiInfo {
    CgiInfo() : server(0) {}
    std::string scriptPath;
    std::string interpreterPath;
    const ServerConfig* server;
    // We can add more info here if needed by CgiExecutor
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
