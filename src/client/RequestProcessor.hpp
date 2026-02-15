#ifndef REQUEST_PROCESSOR_HPP
#define REQUEST_PROCESSOR_HPP

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
  // Retorna true si la petición fue manejada (respuesta lista).
  // Retorna false si es CGI y debe delegarse a Client::startCgiIfNeeded.
  bool process(const HttpRequest& request,
               const std::vector<ServerConfig>* configs, int listenPort,
               int parseErrorCode, HttpResponse& response);

 private:
};

#endif  // REQUEST_PROCESSOR_HPP
