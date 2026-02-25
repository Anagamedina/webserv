#include "ErrorUtils.hpp"

#include <fstream>
#include <sstream>

#include "RequestProcessorUtils.hpp"
#include "ResponseUtils.hpp"

static bool readFileToBody(const std::string& path, std::vector<char>& out) {
  std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open()) return false;
  out.clear();
  char c;
  while (file.get(c)) out.push_back(c);
  return true;
}

void buildErrorResponse(HttpResponse& response, const HttpRequest& request,
                        int statusCode, bool shouldClose,
                        const ServerConfig* server) {
  std::vector<char> body;

  if (server) {
    const ServerConfig::ErrorMap& errorPages = server->getErrorPages();
    ServerConfig::ErrorIterator it = errorPages.find(statusCode);
    if (it != errorPages.end()) {
      std::string errorPath = resolvePath(*server, 0, it->second);
      if (readFileToBody(errorPath, body)) {
        fillBaseResponse(response, request, statusCode, shouldClose, body);
        response.setContentType(it->second);
        return;
      }
    }
  }

  std::ostringstream html;
  html << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
       << "<title>Error " << statusCode << "</title></head><body>"
       << "<h1>Error " << statusCode << "</h1>"
       << "<p>Ocurrió un error en el servidor.</p>"
       << "</body></html>";
  fillBaseResponse(response, request, statusCode, shouldClose,
                   toBody(html.str()));
  response.setHeader("Content-Type", "text/html");
}
