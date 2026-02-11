#ifndef SESSION_UTILS_HPP
#define SESSION_UTILS_HPP

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

/**
 * Adds Set-Cookie: session_id=XXX to the response when the client does not
 * have a valid session. If the request already contains a valid session_id
 * cookie, no Set-Cookie header is added (session reuse).
 * Only adds for 2xx responses (statusCode 200-299).
 */
void addSessionCookieIfNeeded(HttpResponse& response, const HttpRequest& request, int statusCode);

#endif // SESSION_UTILS_HPP
