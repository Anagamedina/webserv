#ifndef RESPONSE_UTILS_HPP
#define RESPONSE_UTILS_HPP

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

#include <string>
#include <vector>

std::vector< char > toBody(const std::string& text);

void fillBaseResponse(HttpResponse& response, const HttpRequest& request, int statusCode,
                      bool shouldClose, const std::vector< char >& body);

#endif // RESPONSE_UTILS_HPP
