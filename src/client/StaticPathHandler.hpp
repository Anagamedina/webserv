#ifndef STATIC_PATH_HANDLER_HPP
#define STATIC_PATH_HANDLER_HPP

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

#include <string>
#include <vector>

bool handleStaticPath(const HttpRequest& request, const ServerConfig* server,
                      const LocationConfig* location, const std::string& path,
                      std::vector< char >& body, HttpResponse& response);

#endif // STATIC_PATH_HANDLER_HPP
