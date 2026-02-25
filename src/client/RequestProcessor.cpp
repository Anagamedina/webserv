#include "RequestProcessor.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"
#include "ResponseUtils.hpp"
#include "StaticPathHandler.hpp"

bool RequestProcessor::handleParseOrMethodErrors(
    const HttpRequest& request, int parseErrorCode, const ServerConfig* server,
    ProcessingResult& result) const {
  if (parseErrorCode == 0 &&
      request.getMethod() != HTTP_METHOD_UNKNOWN) {
    return false;
  }

  int statusCode =
      (parseErrorCode != 0) ? parseErrorCode : HTTP_STATUS_BAD_REQUEST;
  bool shouldClose = true;
  buildErrorResponse(result.response, request, statusCode, shouldClose,
                     server);
  return true;
}

bool RequestProcessor::handleLocationValidation(
    const HttpRequest& request, const ServerConfig* server,
    const LocationConfig* location, bool shouldClose,
    ProcessingResult& result) const {
  int validationCode = validateLocation(request, server, location);
  if (validationCode == 0) {
    return false;
  }

  std::vector<char> body;
  if (validationCode == 301 || validationCode == 302) {
    body.clear();
    fillBaseResponse(result.response, request, validationCode, shouldClose,
                     body);
    result.response.setHeader("Location", location->getRedirectUrl());
  } else {
    buildErrorResponse(result.response, request, validationCode, true, server);
  }

  return true;
}

bool RequestProcessor::handleCgi(const HttpRequest& request,
                                 const ServerConfig* server,
                                 const LocationConfig* location,
                                 const std::string& resolvedPath,
                                 ProcessingResult& result) const {
  std::string interpreterPath;
  if (location) {
    std::string ext = getFileExtension(resolvedPath);
    interpreterPath = location->getCgiPath(ext);
  }

  struct stat st;
  if (stat(resolvedPath.c_str(), &st) != 0) {
    int code = (errno == ENOENT || errno == ENOTDIR) ? HTTP_STATUS_NOT_FOUND
                                                     : HTTP_STATUS_FORBIDDEN;
    buildErrorResponse(result.response, request, code, true, server);
    return true;
  }

  if (!S_ISREG(st.st_mode)) {
    buildErrorResponse(result.response, request, HTTP_STATUS_FORBIDDEN, true,
                       server);
    return true;
  }

  if (interpreterPath.empty()) {
    if (access(resolvedPath.c_str(), X_OK) != 0) {
      int code = (errno == ENOENT || errno == ENOTDIR) ? HTTP_STATUS_NOT_FOUND
                                                       : HTTP_STATUS_FORBIDDEN;
      buildErrorResponse(result.response, request, code, true, server);
      return true;
    }
  } else {
    if (access(resolvedPath.c_str(), R_OK) != 0) {
      int code = (errno == ENOENT || errno == ENOTDIR) ? HTTP_STATUS_NOT_FOUND
                                                       : HTTP_STATUS_FORBIDDEN;
      buildErrorResponse(result.response, request, code, true, server);
      return true;
    }

    if (access(interpreterPath.c_str(), X_OK) != 0) {
      buildErrorResponse(result.response, request,
                         HTTP_STATUS_INTERNAL_SERVER_ERROR, true, server);
      return true;
    }
  }

  result.action = ACTION_EXECUTE_CGI;
  result.cgiInfo.scriptPath = resolvedPath;
  result.cgiInfo.server = server;
  result.cgiInfo.interpreterPath = interpreterPath;
  return true;
}

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

  server = selectServerByPort(listenPort, configs);

  // 1) Errores de parseo / método desconocido
  if (handleParseOrMethodErrors(request, parseErrorCode, server, result)) {
    return result;
  }

  // 2) Seleccionar location adecuada
  if (server) {
    location = matchLocation(*server, request.getPath());
  }

  if (!location) {
    buildErrorResponse(result.response, request, HTTP_STATUS_NOT_FOUND, false,
                       server);
    return result;
  }

  // 3) Validar location (métodos permitidos, redirecciones, etc.)
  if (handleLocationValidation(request, server, location, shouldClose,
                               result)) {
    return result;
  }

  // 4) Resolver ruta física
  resolvedPath = resolvePath(*server, location, request.getPath());
#ifdef DEBUG
  std::cout << " DEBUG: Trying to open: [" << resolvedPath << "]" << std::endl;
#endif

  // 5) Decidir si es CGI
  isCgi = isCgiRequest(resolvedPath) ||
          isCgiRequestByConfig(location, resolvedPath);

  if (isCgi) {
    if (handleCgi(request, server, location, resolvedPath, result)) {
      return result;
    }
  }

  // 6) Contenido estático
  if (handleStaticPath(request, server, location, resolvedPath, body,
                       result.response)) {
    return result;
  }

  // 7) Respuesta base (OK sin body especial)
  fillBaseResponse(result.response, request, statusCode, shouldClose, body);
  return result;
}
