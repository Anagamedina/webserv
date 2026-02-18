#include "RequestProcessor.hpp"

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>

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
// 6) Si es CGI → retorna false para que Client ejecute CgiExecutor
// 7) Si no, servir estático o errores, retorna true
// 6) Decidir respuesta (estático o CGI) + errores
// 7) Rellenar HttpResponse

RequestProcessor::ProcessingResult RequestProcessor::process(
    const HttpRequest& request, const std::vector<ServerConfig>* configs,
    int listenPort, int parseErrorCode) {
  ProcessingResult result;

  int statusCode = HTTP_STATUS_OK;
  std::string resolvedPath = "";
  bool isCgi = false;
  std::vector<char> body;
  bool shouldClose = request.shouldCloseConnection();
  const ServerConfig* server = 0;
  const LocationConfig* location = 0;

  // 1) Errores primero
  if (parseErrorCode != 0 || request.getMethod() == HTTP_METHOD_UNKNOWN) {
    statusCode =
        (parseErrorCode != 0) ? parseErrorCode : HTTP_STATUS_BAD_REQUEST;
    body = toBody(getErrorDescription(statusCode));
    shouldClose = true;
    //fillBaseResponse(response, request, statusCode, shouldClose, body);
    //return true;
    fillBaseResponse(result.response, request, statusCode, shouldClose, body);
    return result;
  }

  // 2) Seleccionar servidor y location
  server = selectServerByPort(listenPort, configs);
  if (server) location = matchLocation(*server, request.getPath());

  if (location) {
    // Validar y resolver
    int validationCode = validateLocation(request, server, location);
    if (validationCode != 0) {
      if (validationCode == 301 || validationCode == 302) {
        body.clear();
        fillBaseResponse(result.response, request, validationCode, shouldClose,
                         body);
        result.response.setHeader("Location", location->getRedirectUrl());
        return result;
      }
      buildErrorResponse(result.response, request, validationCode, true,
                         server);
      return result;
    }

    resolvedPath = resolvePath(*server, location, request.getPath());
#ifdef DEBUG
    std::cout << " DEBUG: Trying to open: [" << resolvedPath << "]"
              << std::endl;
#endif

    isCgi = isCgiRequest(resolvedPath) ||
            isCgiRequestByConfig(location, resolvedPath);

    if (isCgi) {
      std::string interpreterPath;
      if (location) {
        std::string ext = getFileExtension(resolvedPath);
        interpreterPath = location->getCgiPath(ext);
      }

      if (interpreterPath.empty()) {
        struct stat st;
        if (stat(resolvedPath.c_str(), &st) != 0) {
          int code = (errno == ENOENT || errno == ENOTDIR)
                         ? HTTP_STATUS_NOT_FOUND
                         : HTTP_STATUS_FORBIDDEN;
          buildErrorResponse(result.response, request, code, true, server);
          return result;
        }

        if (!S_ISREG(st.st_mode)) {
          buildErrorResponse(result.response, request, HTTP_STATUS_FORBIDDEN,
                             true, server);
          return result;
        }

        if (access(resolvedPath.c_str(), X_OK) != 0) {
          int code = (errno == ENOENT || errno == ENOTDIR)
                         ? HTTP_STATUS_NOT_FOUND
                         : HTTP_STATUS_FORBIDDEN;
          buildErrorResponse(result.response, request, code, true, server);
          return result;
        }
      } else {
        if (access(interpreterPath.c_str(), X_OK) != 0) {
          buildErrorResponse(result.response, request,
                             HTTP_STATUS_INTERNAL_SERVER_ERROR, true, server);
          return result;
        }
      }

      result.action = ACTION_EXECUTE_CGI;
      result.cgiInfo.scriptPath = resolvedPath;
      result.cgiInfo.server = server;
      result.cgiInfo.interpreterPath = interpreterPath;
      return result;
    }

    // Servir archivo estático
                         //response))
    if (handleStaticPath(request, server, location, resolvedPath, body,
                         result.response)) {
      return result;
    }
  } else {
    // 404 Not Found
    buildErrorResponse(result.response, request, 404, false, server);
    return result;
  }

  fillBaseResponse(result.response, request, statusCode, shouldClose, body);
  return result;
}
