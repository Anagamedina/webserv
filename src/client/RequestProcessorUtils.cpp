#include "RequestProcessorUtils.hpp"
#include "http/HttpResponse.hpp"

#include <sys/stat.h>

const ServerConfig* selectServerByPort(
    int port, const std::vector<ServerConfig>* configs) {
  if (configs == 0 || configs->empty()) return 0;

  for (size_t i = 0; i < configs->size(); ++i) {
    if ((*configs)[i].getPort() == port) return &(*configs)[i];
  }
  // comportamiento por defecto: usar el primer server.
  return &(*configs)[0];
}

const LocationConfig* matchLocation(const ServerConfig& server,
                                    const std::string& uri) {
  const std::vector<LocationConfig>& locations = server.getLocations();
  if (locations.empty()) return 0;

  size_t bestLen = 0;
  const LocationConfig* bestLoc = 0;

  for (size_t i = 0; i < locations.size(); ++i) {
    const std::string& path = locations[i].getPath();
    if (path.empty()) continue;
    if (uri.compare(0, path.size(), path) == 0) {
      // If path is "/" or uri is exact match -> match.
      // If path ends with '/' -> match (boundary is implied).
      // If uri has '/' at the end of the path prefix -> match (boundary explicit).
      bool endsWithSlash = (path.size() > 0 && path[path.size() - 1] == '/');

      if (path == "/" || uri.size() == path.size() || endsWithSlash || uri[path.size()] == '/') {
        if (path.size() > bestLen) {
          bestLen = path.size();
          bestLoc = &locations[i];
        }
      }
    } else {
        // Handle case where URI does not end with / but location does,
        // for example:
        // is "/directory" and location is "/directory/"
        // We want to match this so we can later redirect or handle it
        if (path.size() > 1 && path[path.size() - 1] == '/' && 
            uri == path.substr(0, path.size() - 1)) {
            if (path.size() > bestLen) {
                bestLen = path.size();
                bestLoc = &locations[i];
            }
        }
    }
  }

  if (bestLoc) return bestLoc;

  // por defecto, usar la primera location.
  return &locations[0];
}

std::string resolvePath(const ServerConfig& server,
                        const LocationConfig* location,
                        const std::string& uri) {
  std::string root = "./www";
  if (location && !location->getRoot().empty())
    root = location->getRoot();
  else if (!server.getRoot().empty())
    root = server.getRoot();

  std::string locationPath = (location) ? location->getPath() : "/";
  std::string aliasPath;
  std::string rootPath;

  // TODO: revisar si añadir alias dentro de la configuracion (daru no quiere)
  // If URI starts with the location path, replace that prefix with root.
  if (uri.find(locationPath) == 0) {
    aliasPath = root; 
    if (!aliasPath.empty() && aliasPath[aliasPath.size() - 1] != '/')
      aliasPath += "/";

    std::string remainder = uri.substr(locationPath.length());
    if (!remainder.empty() && remainder[0] == '/')
        aliasPath += remainder.substr(1);
    else
        aliasPath += remainder;
  } else if (locationPath.size() > 1 && locationPath[locationPath.size() - 1] == '/' &&
             uri == locationPath.substr(0, locationPath.size() - 1)) {
        aliasPath = root;
        // Strip trailing slash from root if we mapped exactly to directory without slash
        if (!aliasPath.empty() && aliasPath[aliasPath.size() - 1] == '/')
            aliasPath.erase(aliasPath.size() - 1);
  }

  // Standard Root behavior (append URI to root)
  rootPath = root;
  if (!rootPath.empty() && rootPath[rootPath.size() - 1] == '/' && !uri.empty() && uri[0] == '/')
      rootPath.erase(rootPath.size() - 1);
  else if (!rootPath.empty() && rootPath[rootPath.size() - 1] != '/' && !uri.empty() && uri[0] != '/')
      rootPath += "/";
  rootPath += uri;

  // Decision logic:
  // If aliasPath works (exists), use it. 
  // Otherwise use rootPath (standard Nginx behavior).
  struct stat st;
  if (!aliasPath.empty() && stat(aliasPath.c_str(), &st) == 0) {
      return aliasPath;
  }

  return rootPath;
}

bool isCgiRequest(const std::string& path) {
  // Borrador: por ahora, CGI si la extension es .py o .sh
  std::string::size_type dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos) return false;
  std::string ext = path;
  ext.erase(0, dotPos);
  return (ext == ".py" || ext == ".sh");
}

std::string getFileExtension(const std::string& path) {
  std::string::size_type slashPos = path.find_last_of('/');
  std::string::size_type dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos) return "";
  if (slashPos != std::string::npos && dotPos < slashPos) return "";
  return path.substr(dotPos);
}

bool isCgiRequestByConfig(const LocationConfig* location,
                          const std::string& path) {
  if (location == 0) return false;
  std::string ext = getFileExtension(path);
  if (ext.empty()) return false;
  return !location->getCgiPath(ext).empty();
}

std::string methodToString(HttpMethod method) {
  if (method == HTTP_METHOD_GET) return "GET";
  if (method == HTTP_METHOD_POST) return "POST";
  if (method == HTTP_METHOD_DELETE) return "DELETE";
  if (method == HTTP_METHOD_HEAD) return "HEAD";
  return "";
}

int validateLocation(const HttpRequest& request, const ServerConfig* server,
                     const LocationConfig* location) {
  // 1) Redirect -> responder y salir (pendiente de getters de LocationConfig)
  // TODO: check info
  if (!location->getRedirectCode()) return 301;

  // 2) Metodo permitido (HEAD rechazado por defecto a menos que esté explícitamente permitido)
  if (!location->isMethodAllowed(methodToString(request.getMethod())))
    return 405;

  // 3) Body size
  size_t maxBodySize = server ? server->getMaxBodySize() : 0;
  if (location) maxBodySize = location->getMaxBodySize();

  if (maxBodySize > 0 && request.getBody().size() > maxBodySize) return HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE ;

  return 0;
}
