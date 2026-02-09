#include "ErrorUtils.hpp"
#include "ResponseUtils.hpp"

void buildErrorResponse(HttpResponse& response, const HttpRequest& request, int statusCode,
                        bool shouldClose, const ServerConfig* server) {
    // TODO: si hay error_page configurada en server, usarla aqui.
    // TODO: leer archivo de error y usarlo como body.
    (void)server;
    fillBaseResponse(response, request, statusCode, shouldClose, toBody("Error\n"));
}
