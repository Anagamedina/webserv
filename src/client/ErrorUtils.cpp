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




/*
#include "ErrorUtils.hpp"

#include <fstream>
#include <sstream>
#include <unistd.h>

#include "RequestProcessorUtils.hpp"
#include "ResponseUtils.hpp"

static const char* const CONTENT_TYPE_HTML_UTF8 = "text/html; charset=UTF-8";

static bool readFileToBody(const std::string& path, std::vector<char>& out) {
  std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open()) return false;
  out.clear();
  char c;
  while (file.get(c)) out.push_back(c);
  return true;
}

// Intenta leer la página de error; si la ruta es relativa y falla, prueba desde cwd y desde ..
static bool readErrorPageBody(const std::string& errorPath,
                              std::vector<char>& body) {
  if (readFileToBody(errorPath, body) && !body.empty()) return true;
  if (errorPath.empty() || errorPath[0] == '/') return false;
  char cwd[4096];
  if (getcwd(cwd, sizeof(cwd)) == NULL) return false;
  std::string base(cwd);
  std::string altPath =
      (errorPath.size() >= 2 && errorPath[0] == '.' && errorPath[1] == '/')
          ? errorPath.substr(2)
          : errorPath;
  std::string fullPath = base + "/" + altPath;
  if (readFileToBody(fullPath, body) && !body.empty()) return true;
  fullPath = base + "/../" + altPath;
  return readFileToBody(fullPath, body) && !body.empty();
}

static void setFallbackErrorBody(int statusCode, std::vector<char>& body) {
  std::ostringstream html;
  html << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
       << "<title>Error " << statusCode << "</title></head><body>"
       << "<h1>Error " << statusCode << "</h1>"
       << "<p>Ocurrió un error en el servidor.</p>"
       << "</body></html>";
  std::string s = html.str();
  body.assign(s.begin(), s.end());
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
      if (readErrorPageBody(errorPath, body) && !body.empty()) {
        response.setHeader("Content-Type", CONTENT_TYPE_HTML_UTF8);
        fillBaseResponse(response, request, statusCode, shouldClose, body);
        return;
      }
    }
  }

  setFallbackErrorBody(statusCode, body);
  response.setHeader("Content-Type", CONTENT_TYPE_HTML_UTF8);
  fillBaseResponse(response, request, statusCode, shouldClose, body);
}

*/
