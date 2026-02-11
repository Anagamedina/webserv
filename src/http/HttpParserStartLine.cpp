#include "HttpParser.hpp"
/**
 * Extrae una línea completa del buffer interno (_buffer) y la guarda en la
 * variable line.
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
  if (pos == std::string::npos) return false;
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
bool HttpParser::splitStartLine(const std::string& line, std::string& method,
                                std::string& uri, std::string& version) {
  // DEBUG:
  // std::cout << "[splitStartLine] raw line=" << line << std::endl;

  // METHOD SP URI SP VERSION -> 3 partes
  std::string::size_type firstSpace;
  std::string::size_type secondSpace;

  firstSpace = line.find(' ');
  if (firstSpace == std::string::npos) return false;

  secondSpace = line.find(' ', firstSpace + 1);
  if (secondSpace == std::string::npos) return false;

  method = line.substr(0, firstSpace);
  uri = line.substr(firstSpace + 1,
                    secondSpace - firstSpace - 1);  // size de la uri
  version = line.substr(secondSpace + 1);

  // DEBUG:
  // std::cout << "[splitStartLine] method=" << method
  //           << " uri=" << uri
  //           << " version=" << version << std::endl;
  return true;
}

/**
 * Comprueba si el path tiene ".." como segmento (directorio padre).
 * Esto es peligroso: alguien podría pedir /../../etc/passwd para salirse del root.
 *
 * Solo rechazamos cuando ".." va entre barras (o al inicio/final).
 * Ejemplos que SÍ rechazamos: /../, /foo/../bar, ../x
 * Ejemplos que NO rechazamos: file..txt, /foto..jpg (son nombres de archivo normales)
 */
static bool containsParentPathSegment(const std::string& path) {
  std::string::size_type search_pos = 0;

  while (search_pos < path.length()) {
    // Buscar la siguiente aparición de ".."
    std::string::size_type found_at = path.find("..", search_pos);

    if (found_at == std::string::npos) {
      // No hay más "..", el path está bien
      return false;
    }

    // ¿Está el ".." justo al inicio del path o después de una barra?
    bool is_valid_start = (found_at == 0) || (path[found_at - 1] == '/');

    // ¿Termina el ".." al final del path o va seguido de una barra?
    bool is_valid_end = (found_at + 2 >= path.length()) ||
                        (path[found_at + 2] == '/');

    if (is_valid_start && is_valid_end) {
      // Encontramos ".." como segmento de path -> intento de directory traversal
      return true;
    }

    // Seguir buscando por si hay otro ".." más adelante
    search_pos = found_at + 1;
  }

  return false;
}

/**
 * Extrae el path y el query string de la URI.
 * Ejemplo: "/index.html?nombre=ana" -> path="/index.html", query="nombre=ana"
 */
void HttpParser::parseUri(const std::string& uri) {
  std::string path;
  std::string query;

  // Separar path y query por el ?
  // Ejemplo: /index.html?nombre=ana -> path="/index.html", query="nombre=ana"
  std::string::size_type question_mark_pos = uri.find('?');

  if (question_mark_pos != std::string::npos) {
    path = uri.substr(0, question_mark_pos);
    query = uri.substr(question_mark_pos + 1);
  } else {
    path = uri;
    query = "";
  }

  // Seguridad: bloquear intentos de salir del directorio (directory traversal)
  if (containsParentPathSegment(path)) {
    _errorStatusCode = 403;
    _state = ERROR;
    return;
  }

  _request.setPath(path);
  _request.setQuery(query);
}

// PARSE START LINE ----------------------------------------------------------

/**
 * Analiza la línea de inicio de la petición HTTP y la divide en method, uri y
 * version.
 */
void HttpParser::parseStartLine() {
  std::string line;
  std::string method;
  std::string uri;
  std::string version;

  if (!extractLine(line)) return;

  // Ignorar líneas vacías (ej: \r\n al inicio) y esperar la start line real
  while (line.empty()) {
    if (!extractLine(line)) return;
  }

  if (!splitStartLine(line, method, uri, version)) {
    _errorStatusCode = 400;
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
