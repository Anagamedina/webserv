#include "RequestProcessor.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"
#include "ResponseUtils.hpp"
#include "StaticPathHandler.hpp"

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

  if (parseErrorCode != 0 || request.getMethod() == HTTP_METHOD_UNKNOWN) {
    statusCode =
        (parseErrorCode != 0) ? parseErrorCode : HTTP_STATUS_BAD_REQUEST;
    body = toBody(getErrorDescription(statusCode));
    shouldClose = true;
    fillBaseResponse(result.response, request, statusCode, shouldClose, body);
    return result;
  }

  server = selectServerByPort(listenPort, configs);
  if (server) location = matchLocation(*server, request.getPath());

  if (location) {
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

      if (interpreterPath.empty()) {
        if (access(resolvedPath.c_str(), X_OK) != 0) {
          int code = (errno == ENOENT || errno == ENOTDIR)
                         ? HTTP_STATUS_NOT_FOUND
                         : HTTP_STATUS_FORBIDDEN;
          buildErrorResponse(result.response, request, code, true, server);
          return result;
        }
      } else {
        if (access(resolvedPath.c_str(), R_OK) != 0) {
          int code = (errno == ENOENT || errno == ENOTDIR)
                         ? HTTP_STATUS_NOT_FOUND
                         : HTTP_STATUS_FORBIDDEN;
          buildErrorResponse(result.response, request, code, true, server);
          return result;
        }

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

    if (handleStaticPath(request, server, location, resolvedPath, body,
                         result.response)) {
      return result;
    }
  } else {
    buildErrorResponse(result.response, request, 404, false, server);
    return result;
  }

  fillBaseResponse(result.response, request, statusCode, shouldClose, body);
  return result;
}
