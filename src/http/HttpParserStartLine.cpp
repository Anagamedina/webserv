#include "HttpParser.hpp"
/**
 * Extrae una línea completa del buffer interno (_buffer) y la guarda en la variable line.
 * @param line: La línea extraída del buffer.
 * @return: true si se extrajo la línea, false si no.
 * ejemplo de buffer: GET /index.html HTTP/1.1\r\nHost: www.example.com\r\n\r\n
 * extractLine devuelve GET /index.html HTTP/1.1
 * borra el buffer "GET /index.html HTTP/1.1\r\n"
 * buffer queda "Host: www.example.com\r\n\r\n"
 * y line queda "GET /index.html HTTP/1.1"
 * y el estado del parser es PARSING_HEADERS ???
 */
bool HttpParser::extractLine(std::string& line) {
    std::string::size_type pos;

    pos = _buffer.find("\r\n");
    if (pos == std::string::npos)
        return false;
    // si no la borras, la procesarías otra vez en la siguiente llamada.
    // Borrarla significa: “ya está consumida”,
    line = _buffer.substr(0, pos);
    _buffer.erase(0, pos + 2);
    // DEBUG:
    // std::cout << "[extractLine] line=" << line << std::endl;
    return true;
}

/**
 * Divide la línea de inicio de la petición HTTP en method, uri y version.
 * @param line: La línea de inicio de la petición HTTP.
 * @param method: El método HTTP.
 * @param uri: La URI.
 * @param version: La versión de HTTP.
 * @return: true si se dividió la línea correctamente, false si no.
 * ejemplo: line : GET /index.html?query=value HTTP/1.1 METHOD SP URI SP VERSION
 * method: "GET" firstSpace: 3
 * uri: "/index.html" secondSpace: 27
 * version: "HTTP/1.1"
 */
bool HttpParser::splitStartLine(const std::string& line, std::string& method, std::string& uri,
                                std::string& version) {
    // DEBUG:
    // std::cout << "[splitStartLine] raw line=" << line << std::endl;

    // METHOD SP URI SP VERSION -> 3 partes
    std::string::size_type firstSpace;
    std::string::size_type secondSpace;

    firstSpace = line.find(' ');
    if (firstSpace == std::string::npos)
        return false;

    secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
        return false;

    method = line.substr(0, firstSpace);
    uri = line.substr(firstSpace + 1, secondSpace - firstSpace - 1); // size de la uri
    version = line.substr(secondSpace + 1);

    // DEBUG:
    // std::cout << "[splitStartLine] method=" << method
    //           << " uri=" << uri
    //           << " version=" << version << std::endl;
    return true;
}

/**
 * Analiza la URI y la divide en path y query.
 * @param uri: La URI a analizar.
 * ejemplo: uri: "/index.html?query=value"
 * qpos: 14
 * path: "/index.html" qpos: 14
 * query: "query=value"
 */
void HttpParser::parseUri(const std::string& uri) {
    std::string::size_type qpos;

    qpos = uri.find('?');
    if (qpos != std::string::npos) {
        _request.setPath(uri.substr(0, qpos));
        _request.setQuery(uri.substr(qpos + 1));
    } else {
        _request.setPath(uri);
        _request.setQuery("");
    }
}

// PARSE START LINE ----------------------------------------------------------

/**
 * Analiza la línea de inicio de la petición HTTP y la divide en method, uri y version.
 */
void HttpParser::parseStartLine() {
    std::string line;
    std::string method;
    std::string uri;
    std::string version;

    if (!extractLine(line))
        return;

    // DEBUG:
    // std::cout << "[parseStartLine] line=" << line << std::endl;

    if (line.empty()) {
        _state = ERROR;
        return;
    }

    if (!splitStartLine(line, method, uri, version)) {
        _state = ERROR;
        return;
    }

    // Porque HttpRequest es la estructura que guarda la petición ya parseada.
    _request.setMethod(method);
    _request.setVersion(version);
    parseUri(uri);

    // actualizo el estado
    //  DEBUG:
    //  std::cout << "[parseStartLine] state -> PARSING_HEADERS" << std::endl;
    _state = PARSING_HEADERS;
}
