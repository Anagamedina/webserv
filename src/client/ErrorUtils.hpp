#ifndef ERROR_UTILS_HPP
#define ERROR_UTILS_HPP

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../config/ServerConfig.hpp"

void buildErrorResponse(HttpResponse& response,
						const HttpRequest& request,
						int statusCode,
						bool shouldClose,
						const ServerConfig* server);

#endif // ERROR_UTILS_HPP
