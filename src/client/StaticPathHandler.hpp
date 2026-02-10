#ifndef STATIC_PATH_HANDLER_HPP
#define STATIC_PATH_HANDLER_HPP

#include <string>
#include <vector>

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

bool handleStaticPath(const HttpRequest& request, const ServerConfig* server,
                      const LocationConfig* location, const std::string& path,
                      std::vector<char>& body, HttpResponse& response);

#endif  // STATIC_PATH_HANDLER_HPP
