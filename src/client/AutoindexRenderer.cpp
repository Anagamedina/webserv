#include "AutoindexRenderer.hpp"

#include <sstream>

std::vector<char> renderAutoindexHtml(const std::string& base,
                                      const std::string& itemsHtml) {
  std::ostringstream html;
  std::string safeBase = base;
  if (safeBase.empty()) safeBase = "/";

  html << "<!DOCTYPE html>\n"
       << "<html>\n"
       << "  <head>\n"
       << "    <meta charset=\"utf-8\">\n"
       << "    <title>Index of " << safeBase << "</title>\n"
       << "  </head>\n"
       << "  <body>\n"
       << "    <h1>Index of " << safeBase << "</h1>\n"
       << "    <ul>\n"
       << itemsHtml << "    </ul>\n"
       << "  </body>\n"
       << "</html>\n";

  std::string content = html.str();
  return std::vector<char>(content.begin(), content.end());
}

/*#include "ErrorUtils.hpp"
#include "ResponseUtils.hpp"
#include "StaticPathHandler.hpp" // Para usar readFileToBody
#include <sstream>

// Función para generar un HTML de "emergencia" con estilo similar al Autoindex
static std::vector<char> getFallbackErrorBody(int statusCode) {
    std::ostringstream html;

    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "  <head>\n"
         << "    <meta charset=\"utf-8\">\n"
         << "    <title>Error " << statusCode << "</title>\n"
         << "    <style>\n"
         << "      body { font-family: sans-serif; text-align: center; padding:
50px; }\n"
         << "      h1 { color: #333; }\n"
         << "      hr { width: 50%; border: 0; border-top: 1px solid #eee; }\n"
         << "    </style>\n"
         << "  </head>\n"
         << "  <body>\n"
         << "    <h1>" << statusCode << " " << getStatusText(statusCode) <<
"</h1>\n"
         << "    <hr>\n"
         << "    <p>Webserv/1.0 (42 Barcelona)</p>\n"
         << "  </body>\n"
         << "</html>\n";

    std::string content = html.str();
    return std::vector<char>(content.begin(), content.end());
}

void buildErrorResponse(HttpResponse& response, const HttpRequest& request, int
statusCode, bool shouldClose, const ServerConfig* server) {

    std::vector<char> body;
    bool customPageFound = false;

    // 1. Intentamos buscar la página personalizada en la configuración de Daru
    if (server) {
        // Asumiendo que Daru implementa este método que devuelve el path del
.html std::string customPath = server->getErrorPagePath(statusCode);

        if (!customPath.empty()) {
            // Intentamos leer el archivo físico (www/errors/404.html, etc.)
            if (readFileToBody(customPath, body)) {
                customPageFound = true;
            }
        }
    }

    // 2. Si no hay página personalizada o no se pudo leer el archivo, usamos el
fallback if (!customPageFound) { body = getFallbackErrorBody(statusCode);
    }

    // 3. Rellenamos la respuesta final
    fillBaseResponse(response, request, statusCode, shouldClose, body);

    // Forzamos el Content-Type a HTML para que el navegador no lo descargue
    response.setHeader("Content-Type", "text/html");
}*/
