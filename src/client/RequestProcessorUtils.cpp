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
  if (!path.empty() && path[path.size() - 1] == '/' && !uri.empty() &&
      uri[0] == '/')
    path.erase(path.size() - 1);
  else if (!path.empty() && path[path.size() - 1] != '/' && !uri.empty() &&
           uri[0] != '/')
    path += "/";
  path += uri;
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

  // 2) Metodo permitido
  if (!location->isMethodAllowed(methodToString(request.getMethod())))
    return 405;

  // 3) Body size (usar limite del server por ahora)
  if (server && request.getBody().size() > server->getMaxBodySize()) return 413;

  return 0;
}
