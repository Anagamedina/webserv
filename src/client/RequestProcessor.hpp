#ifndef REQUEST_PROCESSOR_HPP
#define REQUEST_PROCESSOR_HPP

#include <vector>

#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

// El cerebro del servidor: es quien decide que hacer con la peticion dado un HttpRequest y la
// configuración, construye un HttpResponse (estático, CGI, error, etc.) y lo devuelve al cliente.
// solo transforma un HttpRequest en HttpResponse sin enviarlo al cliente.
class RequestProcessor {
  public:
    void process(const HttpRequest& request, const std::vector< ServerConfig >* configs,
                 int listenPort, bool parseError, HttpResponse& response);

  private:
};

#endif // REQUEST_PROCESSOR_HPP
