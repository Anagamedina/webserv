#ifndef SESSION_UTILS_HPP
#define SESSION_UTILS_HPP

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

// si el cliente no tiene cookie "id" valida, le mandamos una nueva con Set-Cookie
// solo se usa en respuestas 200-299
void addSessionCookieIfNeeded(HttpResponse& response, const HttpRequest& request, int statusCode);

#endif // SESSION_UTILS_HPP
