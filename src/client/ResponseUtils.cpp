#include "ResponseUtils.hpp"

static std::string versionToString(HttpVersion version) {
    if (version == HTTP_VERSION_1_0)
        return "HTTP/1.0";
    return "HTTP/1.1";
}

std::vector< char > toBody(const std::string& text) {
    return std::vector< char >(text.begin(), text.end());
}

void fillBaseResponse(HttpResponse& response, const HttpRequest& request, int statusCode,
                      bool shouldClose, const std::vector< char >& body) {
    response.setStatusCode(statusCode);
    response.setVersion(versionToString(request.getVersion()));
    if (shouldClose)
        response.setHeader("Connection", "close");
    else
        response.setHeader("Connection", "keep-alive");
    response.setContentType(request.getPath());
    response.setBody(body);
}
