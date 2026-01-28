#include "RequestProcessor.hpp"

static std::string httpVersionToString(HttpVersion version)
{
    if (version == HTTP_VERSION_1_0)
        return "HTTP/1.0";
    if (version == HTTP_VERSION_1_1)
        return "HTTP/1.1";
    return "HTTP/1.1";
}

namespace {
static std::vector<char> toBody(const std::string& text)
{
    return std::vector<char>(text.begin(), text.end());
}
//llena la respuesta base con el status code, la version, el connection, el content type y el body.
static void fillBaseResponse(HttpResponse& response,
                             const HttpRequest& request,
                             int statusCode,
                             bool shouldClose,
                             const std::vector<char>& body)
{
    response.setStatusCode(statusCode);
    response.setVersion(httpVersionToString(request.getVersion()));
    if (shouldClose)
        response.setHeader("Connection", "close");
    else
        response.setHeader("Connection", "keep-alive");
    response.setContentType(request.getPath());
    response.setBody(body);
}

} // namespace

void RequestProcessor::process(const HttpRequest& request,
                               const std::vector<ServerBlock>* configs,
                               bool parseError,
                               HttpResponse& response)
{
    int statusCode = HTTP_STATUS_OK;
    // TODO: reemplazar body fijo por contenido real (archivo o CGI).
    std::vector<char> body = toBody("OK\n");
    bool shouldClose = request.shouldCloseConnection();

    (void)configs;
    if (parseError || request.getMethod() == HTTP_METHOD_UNKNOWN)
    {
        statusCode = HTTP_STATUS_BAD_REQUEST;
        body = toBody("Bad Request\n");
        shouldClose = true;
    }

    // TODO: integrar CGI: si la location es CGI, delegar en CgiHandler (Carles).
    // TODO: respuesta estatica: root + path, access/stat, leer archivo.
    fillBaseResponse(response, request, statusCode, shouldClose, body);
}
