#ifndef ERROR_UTILS_HPP
#define ERROR_UTILS_HPP

#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

void buildErrorResponse(HttpResponse& response, const HttpRequest& request, int statusCode,
                        bool shouldClose, const ServerConfig* server);

#endif // ERROR_UTILS_HPP
