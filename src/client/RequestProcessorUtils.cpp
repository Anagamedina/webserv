#include "RequestProcessorUtils.hpp"

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
      if (path == "/" || uri.size() == path.size() || uri[path.size()] == '/') {
        if (path.size() > bestLen) {
          bestLen = path.size();
          bestLoc = &locations[i];
        }
      }
    } else {
        // Handle case where URI is "/directory" and location is "/directory/"
        // We want to match this so we can later redirect or handle it
        if (path.size() > 1 && path[path.size() - 1] == '/' && 
            uri == path.substr(0, path.size() - 1)) {
            if (path.size() > bestLen) {
                bestLen = path.size(); // Use the full path length as metric
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

  std::string path = root;
  std::string locationPath = (location) ? location->getPath() : "/";

  // If URI starts with the location path, replace that prefix with root.
  // This behaves like Nginx 'alias' directive, which is often required
  // by 42 testers/subjects even when 'root' is used in config.
  if (uri.find(locationPath) == 0) {
    if (!path.empty() && path[path.size() - 1] != '/')
      path += "/";
    
    std::string remainder = uri.substr(locationPath.length());
    if (!remainder.empty() && remainder[0] == '/')
      path += remainder.substr(1);
    else
      path += remainder;
  } 
  // Handle case: URI is "/directory", location is "/directory/"
  else if (locationPath.size() > 1 && locationPath[locationPath.size() - 1] == '/' &&
           uri == locationPath.substr(0, locationPath.size() - 1)) {
      // Map exactly to root (which is the alias target)
      path = root;
      // Ensure we don't end up with trailing slash if root doesn't have one?
      // Actually, if it's a directory, having slash is fine or not.
      // But typically "path" is the file system path.
  }
  else {
    // Fallback: standard append behavior (should usually not happen if matched)
    if (!path.empty() && path[path.size() - 1] == '/' && !uri.empty() &&
        uri[0] == '/')
      path.erase(path.size() - 1);
    else if (!path.empty() && path[path.size() - 1] != '/' && !uri.empty() &&
             uri[0] != '/')
      path += "/";
    path += uri;
  }
  return path;
}

bool isCgiRequest(const std::string& path) {
  // Borrador: por ahora, CGI si la extension es .py o .php
  std::string::size_type dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos) return false;
  std::string ext = path;
  ext.erase(0, dotPos);
  return (ext == ".py" || ext == ".php");
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

  // 3) Body size (usar limite del server por ahora)
  if (server && request.getBody().size() > server->getMaxBodySize()) return 413;

  return 0;
}
