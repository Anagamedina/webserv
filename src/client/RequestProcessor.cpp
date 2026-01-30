#include "RequestProcessor.hpp"
#include "../config/LocationConfig.hpp"


/**
*El cliente puede enviarte HTTP/1.0.
*La norma pide responder con la misma versión que el request (si la soportas).
*Si recibes una versión que no soportas, debes responder error (típico 505 HTTP Version Not Supported).
*/

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

//Helper function to strip the port from the host header.
//ej: "localhost:8080" -> "localhost"
//ej: "localhost" -> "localhost" (no tiene puerto)
static std::string stripPortFromHost(const std::string& hostHeader)
{
    std::string result_header = hostHeader;
    std::string::size_type colon = result_header.find(':');
    if (colon != std::string::npos)
        result_header.erase(colon);
    return (result_header);   
}

static const ServerConfig* selectServerByHost(const std::string& hostHeader,
                                              const std::vector<ServerConfig>* configs)
{
    if (configs == 0 || configs->empty())
        return 0;

    std::string host = stripPortFromHost(hostHeader);
    for (size_t i = 0; i < configs->size(); ++i) {
        if ((*configs)[i].getServerName() == host)
            return &(*configs)[i];
    }
//comportamiento por defecto: usar el primer server.
    return &(*configs)[0];
}

static const ServerConfig* selectServerByPort(int port,
                                              const std::vector<ServerConfig>* configs)
{
    if (configs == 0 || configs->empty())
        return 0;

    for (size_t i = 0; i < configs->size(); ++i) {
        if ((*configs)[i].getPort() == port)
            return &(*configs)[i];
    }
    // comportamiento por defecto: usar el primer server.
    return &(*configs)[0];
}

static const LocationConfig* matchLocation(const ServerConfig& server,
                                           const std::string& uri)
{
    const std::vector<LocationConfig>& locations = server.getLocations();
    if (locations.empty())
        return 0;

    // TODO: elegir la location con el mejor match de path.
    // Falta acceso a getters de LocationConfig para comparar path.
    // Cuando existan getters:
    // const std::string& path = locations[i].getPath();
    // const std::string& root = locations[i].getRoot();
    // const std::string& index = locations[i].getIndex();
    // bool autoindex = locations[i].getAutoindex();
    // const std::vector<std::string>& methods = locations[i].getMethods();
    // const std::string& upload = locations[i].getUploadStore();
    // const std::string& redirect = locations[i].getRedirect();
    
    //PSEUDOCODIGO MATCH LOCATION : mejor match de path.
    // const std::map<std::string, std::string>& cgi = locations[i].getCgiHandlers();

    // Pseudologica de match:
    // size_t best = 0; const LocationConfig* bestLoc = 0;
    // for cada location:
    //   if uri empieza por path y (path == "/" o uri[path.size()] == '/'):
    //       si path.size() > best -> best = path.size(); bestLoc = &location;
    // return bestLoc;
    (void)uri;
    return &locations[0];
}

//funcion auxiliar para verificar si la peticion es un request CGI.
//ej: "/index.php" -> true
//ej: "/index.html" -> false
// dotPos es la posicion del ultimo punto en la ruta.
// ext es la extension de la ruta.
// ej: "/index.php" -> "php"
// ej: "/index.html" -> "html"
static bool isCgiRequest(const std::string& path)
{
    // Borrador: por ahora, CGI si la extension es .py o .php
    std::string::size_type dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos)
        return false;
    std::string ext = path;
    ext.erase(0, dotPos);
    return (ext == ".py" || ext == ".php");
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







//MAIN FUNCTION: LOGIC OF THE PROCESSOR.
    // Flujo general:
    // 1) Inicializar status/body/shouldClose
    // 2) Matching virtual host (ServerConfig por Host)
    // 3) Matching location (LocationConfig por URI)
    // 4) Validaciones (metodo, body size, redirect)
    // 5) Resolver path real (root/alias + uri)
    // 6) Decidir respuesta (estatico o CGI) + errores
    // 7) Rellenar HttpResponse
void RequestProcessor::process(const HttpRequest& request,
                               const std::vector<ServerConfig>* configs,
                               int listenPort,
                               bool parseError,
                               HttpResponse& response)
{
    int statusCode = HTTP_STATUS_OK;
    // TODO: reemplazar body fijo por contenido real (archivo o CGI).
    std::vector<char> body = toBody("OK\n");
    bool shouldClose = request.shouldCloseConnection();

    // Primero por puerto (obligatorio), luego por Host si hace falta.
    const ServerConfig* server = selectServerByPort(listenPort, configs);
    // Host como extra (opcional):
    // if (server == 0) {
    //     server = selectServerByHost(request.getHeader("Host"), configs);
    // }
    const LocationConfig* location = 0;
    if (server)
        location = matchLocation(*server, request.getPath());
    (void)location;
    if (parseError || request.getMethod() == HTTP_METHOD_UNKNOWN)
    {
        statusCode = HTTP_STATUS_BAD_REQUEST;
        body = toBody("Bad Request\n");
        shouldClose = true;
    }

    // TODO: integrar CGI: si la location es CGI, delegar en CgiHandler (Carles).
    // TODO: respuesta estatica: root + path, access/stat, leer archivo.
    bool isCgi = isCgiRequest(request.getPath());
    (void)isCgi;
    // TODO: pseudologica de decision estatica vs CGI (cuando LocationConfig tenga getters):
    // if (location) {
    //   if (!location->getRedirect().empty()) -> 301/302
    //   if (!location->isMethodAllowed(request.getMethod())) -> 405
    //   if (existe CGI para la extension) -> CgiHandler
    //   else -> servir estatico (root + uri)
    // }
    fillBaseResponse(response, request, statusCode, shouldClose, body);
}
