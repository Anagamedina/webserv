#include "ResponseUtils.hpp"

#include "SessionUtils.hpp"

static std::string versionToString(HttpVersion version) {
  if (version == HTTP_VERSION_1_0) return "HTTP/1.0";
  return "HTTP/1.1";
}

std::vector<char> toBody(const std::string& text) {
  return std::vector<char>(text.begin(), text.end());
}

std::string getErrorDescription(int statusCode) {
  if (statusCode == HTTP_STATUS_FORBIDDEN) return "Forbidden\n";
  if (statusCode == HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE)
    return "Request Entity Too Large\n";
  return "Bad Request\n";  // Por defecto para 400 u otros errores de parseo
}


/*
 * @brief Fill a base response.
 * 
 * Fill a base response.
 * 
 * @param response The response to fill.
 * @param request The request.
 * @param statusCode The status code.
 * @param shouldClose Whether to close the connection.
 * @param body The body to fill the response with.
 * 
 */
void fillBaseResponse(HttpResponse& response, const HttpRequest& request,
                      int statusCode, bool shouldClose,
                      const std::vector<char>& body) {
  response.setStatusCode(statusCode);
  response.setVersion(versionToString(request.getVersion()));
  if (shouldClose)
    response.setHeader("Connection", "close");
  else
    response.setHeader("Connection", "keep-alive");
  if (!response.hasHeader("content-type"))
    response.setContentType(request.getPath());
  response.setBody(body);
  if (request.getMethod() == HTTP_METHOD_HEAD) {
    response.setHeadOnly(true);
  }

  addSessionCookieIfNeeded(response, request, statusCode);
}
