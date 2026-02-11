#ifndef RESPONSE_UTILS_HPP
#define RESPONSE_UTILS_HPP

#include <string>
#include <vector>

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

std::vector<char> toBody(const std::string& text);

std::string getErrorDescription(int statusCode);

void fillBaseResponse(HttpResponse& response, const HttpRequest& request,
                      int statusCode, bool shouldClose,
                      const std::vector<char>& body);

#endif  // RESPONSE_UTILS_HPP
