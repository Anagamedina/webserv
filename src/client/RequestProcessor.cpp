#include "RequestProcessor.hpp"

#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"
#include "ResponseUtils.hpp"
#include "StaticPathHandler.hpp"

// Función principal del procesador de peticiones.
// Flujo general:
// 1) Inicializar status, body, shouldClose
// 2) Matching virtual host (ServerConfig por puerto)
// 3) Matching location (LocationConfig por URI)
// 4) Validaciones (método, tamaño body, redirect)
// 5) Resolver path real (root/alias + uri)
// 6) Decidir respuesta (estático o CGI) + errores
// 7) Rellenar HttpResponse
void RequestProcessor::process(const HttpRequest& request,
                               const std::vector<ServerConfig>* configs,
                               int listenPort, int parseErrorCode,
                               HttpResponse& response) {
  int statusCode = HTTP_STATUS_OK;
  std::string resolvedPath = "";
  bool isCgi = false;
  std::vector<char> body;
  bool shouldClose = request.shouldCloseConnection();
  const ServerConfig* server = 0;
  const LocationConfig* location = 0;

  // 1) Errores primero: si el parser falló, respondemos con el código adecuado
  if (parseErrorCode != 0 || request.getMethod() == HTTP_METHOD_UNKNOWN) {
    statusCode = (parseErrorCode != 0) ? parseErrorCode : HTTP_STATUS_BAD_REQUEST;
    body = toBody(getErrorDescription(statusCode));
    shouldClose = true;
    fillBaseResponse(response, request, statusCode, shouldClose, body);
    return;
  }

  // 2) Seleccionar servidor por puerto y buscar location que coincida con el path
  server = selectServerByPort(listenPort, configs);
  if (server) location = matchLocation(*server, request.getPath());

  if (location) {
    // Hay location: validar y resolver la ruta real en disco
    int validationCode = validateLocation(request, server, location);
    if (validationCode != 0) {
      if (validationCode == 301 || validationCode == 302) {
        // Redirect: enviar respuesta con header Location
        body.clear();
        fillBaseResponse(response, request, validationCode, shouldClose, body);
        response.setHeader("Location", location->getRedirectUrl());
        return;
      }
      buildErrorResponse(response, request, validationCode, true, server);
      return;
    }

    resolvedPath = resolvePath(*server, location, request.getPath());
    std::cout << " DEBUG: Intentando abrir: [" << resolvedPath << "]"
              << std::endl;
    isCgi =
        isCgiRequest(resolvedPath) || isCgiRequestByConfig(location, resolvedPath);

    if (isCgi) {
      // CGI no implementado aún -> 501
      buildErrorResponse(response, request, 501, true, server);
      return;
    }

    // Servir archivo estático (o error 403/404)
    if (handleStaticPath(request, server, location, resolvedPath, body,
                         response))
      return;
  } else {
    // No hay location que coincida -> 404
    buildErrorResponse(response, request, 404, false, server);
    return;
  }

  fillBaseResponse(response, request, statusCode, shouldClose, body);
}
