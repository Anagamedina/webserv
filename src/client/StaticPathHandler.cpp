#include "StaticPathHandler.hpp"

#include "AutoindexRenderer.hpp"
#include "ErrorUtils.hpp"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

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
    // std::vector<std::string> indexes = server->getIndexVector();
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
    for (size_t i = 0; i < indexes.size(); ++i) {
        indexPath = path;
        if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
            indexPath += "/";
        indexPath += indexes[i];

        bool isDir = false;
        bool isReg = false;
        if (getPathInfo(indexPath, isDir, isReg) && isReg) {
            foundIndex = true;
            break;
        }
    }

    if (foundIndex) {
        if (!readFileToBody(indexPath, body)) {
            buildErrorResponse(response, request, 403, false, server);
            return true;
        }
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
        buildErrorResponse(response, request, 403, false, server);
        return true;
    }

    return false;
}

bool handleStaticPath(const HttpRequest& request, const ServerConfig* server,
                      const LocationConfig* location, const std::string& path,
                      std::vector< char >& body, HttpResponse& response) {
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
