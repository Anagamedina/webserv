#include "StaticPathHandler.hpp"

#include "AutoindexRenderer.hpp"
#include "ErrorUtils.hpp"
#include "RequestProcessorUtils.hpp"
#include "ResponseUtils.hpp"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <ctime>

static bool readFileToBody(const std::string& path, std::vector< char >& out) {

    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        return false;

    out.clear();
    char c;
    while (file.get(c))
        out.push_back(c);
    return true;
}

static bool getPathInfo(const std::string& path, bool& isDir, bool& isReg) {

    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    isDir = S_ISDIR(st.st_mode);
    isReg = S_ISREG(st.st_mode);
    return true;
}

static std::vector< char > generateAutoIndexBody(const std::string& dirPath,
                                                 const std::string& requestPath) {
    std::string base = requestPath;
    if (base.empty())
        base = "/";
    if (base[base.size() - 1] != '/')
        base += "/";

    std::ostringstream items;

    DIR* dir = opendir(dirPath.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;

            std::string fullPath = dirPath;
            if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
                fullPath += "/";
            fullPath += name;

            struct stat st;
            bool isDir = (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

            items << "      <li><a href=\"" << base << name;
            if (isDir)
                items << "/";
            items << "\">" << name;
            if (isDir)
                items << "/";
            items << "</a></li>\n";
        }
        closedir(dir);
    }

    return renderAutoindexHtml(base, items.str());
}

static bool handleDirectory(const HttpRequest& request, const ServerConfig* server,
                            const LocationConfig* location, const std::string& path,
                            std::vector< char >& body, HttpResponse& response) {
    
    std::vector< std::string > indexes;

    if (location) {
        indexes = location->getIndexes();
    }
    if (indexes.empty() && server && !server->getIndexVector().empty()) {
        indexes = server->getIndexVector();
    }
    if (indexes.empty()) {
        indexes.push_back("index.html");
    }

    bool foundIndex = false;
    std::string indexPath;
    std::string indexName;
    for (size_t i = 0; i < indexes.size(); ++i) {
        indexPath = path;
        if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
            indexPath += "/";
        indexPath += indexes[i];
        indexName = indexes[i];

        bool isDir = false;
        bool isReg = false;
        if (getPathInfo(indexPath, isDir, isReg) && isReg) {
            foundIndex = true;
            break;
        }
    }
// 
    if (foundIndex) {
        // Double check: si el index es CGI segun config, no servir como estatico.
        if (isCgiRequestByConfig(location, indexPath)) {
            std::string redirectPath = request.getPath();
            if (redirectPath.empty())
                redirectPath = "/";
            if (redirectPath[redirectPath.size() - 1] != '/')
                redirectPath += "/";
            redirectPath += indexName;

            std::vector< char > empty;
            fillBaseResponse(response, request, 302, request.shouldCloseConnection(), empty);
            response.setHeader("Location", redirectPath);
            return true;
        }
        if (!readFileToBody(indexPath, body)) {
            // Si no se puede abrir el archivo (no existe o sin permisos de lectura),
            // devolvemos 403. No hace falta comprobar permisos con stat/access aparte.
            buildErrorResponse(response, request, 403, false, server);
            return true;
        }
        // El index es un archivo estatico normal: fijamos el Content-Type
        // usando la extension real del fichero (index.html, index.css, ...).
        response.setContentType(indexPath);
        return false;
    }

    if (location && location->getAutoIndex()) {
        body = generateAutoIndexBody(path, request.getPath());
        response.setHeader("Content-Type", "text/html");
        return false;
    }

    buildErrorResponse(response, request, 403, false, server);
    return true;
}

static bool handleRegularFile(const HttpRequest& request, const ServerConfig* server,
                              const std::string& path, std::vector< char >& body,
                              HttpResponse& response) {

    if (request.getMethod() == HTTP_METHOD_POST) {
        buildErrorResponse(response, request, 405, false, server);
        return true;
    }
    if (request.getMethod() == HTTP_METHOD_DELETE) {
        if (unlink(path.c_str()) == 0) {
            body.clear();
            return false;
        }
        buildErrorResponse(response, request, 500, true, server);
        return true;
    }

    if (!readFileToBody(path, body)) {
        // Mismo criterio que para el index: si ifstream no puede abrir el archivo
        // (por permisos o porque ha desaparecido), respondemos 403 Forbidden.
        buildErrorResponse(response, request, 403, false, server);
        return true;
    }
    // Archivo estatico leido correctamente: asignamos Content-Type segun su extension.
    response.setContentType(path);
    return false;
}

static bool handleUpload(const HttpRequest& request, const ServerConfig* server,
                         const LocationConfig* location, const std::string& path,
                         std::vector< char >& body, HttpResponse& response) {
    (void)body;
    std::string uploadStore = location->getUploadStore();
    if (uploadStore.empty()) {
        buildErrorResponse(response, request, HTTP_STATUS_METHOD_NOT_ALLOWED, false, server);
        return true;
    }

    // Intentamos usar el ultimo segmento de la ruta como nombre de archivo.
    // Ej: POST /uploads/mi_foto.png -> filename = "mi_foto.png"
    std::string filename;
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash < path.length() - 1) {
        filename = path.substr(lastSlash + 1);
    } else {
        // Si no viene nombre en la URL, generamos uno simple con timestamp.
        std::ostringstream oss;
        oss << "uploaded_file_" << std::time(NULL);
        filename = oss.str();
    }

    if (!uploadStore.empty() && uploadStore[uploadStore.length() - 1] != '/') {
        uploadStore += "/";
    }

    std::string fullPath = uploadStore + filename;

    std::ofstream outFile(fullPath.c_str(), std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        buildErrorResponse(response, request, HTTP_STATUS_INTERNAL_SERVER_ERROR, true, server);
        return true;
    }

    const std::vector< char >& reqBody = request.getBody();
    if (!reqBody.empty()) {
        outFile.write(&reqBody[0], reqBody.size());
    }
    outFile.close();

    response.setStatusCode(HTTP_STATUS_CREATED);
    // Podemos opcionalmente devolver un pequeño mensaje en el body:
    // body = toBody("File uploaded\n");
    return true;
}

bool handleStaticPath(const HttpRequest& request, const ServerConfig* server,
                      const LocationConfig* location, const std::string& path,
                      std::vector< char >& body, HttpResponse& response) {
    // Si es un POST y la location tiene configurado un upload_store, manejamos
    // la subida de archivo en lugar de servir un recurso estatico normal.
    if (request.getMethod() == HTTP_METHOD_POST && location &&
        !location->getUploadStore().empty()) {
        return handleUpload(request, server, location, path, body, response);
    }

    bool isDir = false;
    bool isReg = false;
    if (!getPathInfo(path, isDir, isReg)) {
        buildErrorResponse(response, request, 404, false, server);
        return true;
    }

    if (isDir)
        return handleDirectory(request, server, location, path, body, response);

    if (!isReg) {
        buildErrorResponse(response, request, 403, false, server);
        return true;
    }

    return handleRegularFile(request, server, path, body, response);
}
