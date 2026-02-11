#include <cstdlib>

#include "HttpHeaderUtils.hpp"
#include "HttpParser.hpp"

// Helpers moved to HttpHeaderUtils

/**
 * @brief Divide una linea de header en key y value
 *
 */
bool HttpParser::splitHeaderLine(const std::string& line, std::string& key,
                                 std::string& value) {
  return http_header_utils::splitHeaderLine(line, key, value);
}

/**
 * se encarga de manejar los headers de la peticion y
 *   añadirlos al mapa de headers del request ademas de
 *   manejar el content-length y el transfer-encoding activando
 *   el la flag de chunk si es necesario
 * @param key: La key del header
 * @param value: El valor del header
 */
void HttpParser::handleHeader(const std::string& key,
                              const std::string& value) {
  std::string keyLower;
  std::string valueLower;

  // Normalizamos una sola vez y usamos la misma clave para:
  // 1) guardar en el mapa
  // 2) comparar
  keyLower = http_header_utils::toLowerCopy(key);
  _request.addHeaders(keyLower, value);
  if (keyLower == "content-length") {
    // convierte el valor a un numero entero
    // cuántos bytes exactos debe esperar antes de marcar la petición como
    // COMPLETE.
    _contentLength = std::strtoul(value.c_str(), 0, 10);
    return;
  }

  if (keyLower == "transfer-encoding") {
    // Esto hará que tu parser ignore el Content-Length y use la lógica
    // de los "vagones" (hexadecimales) que programaste en parseBodyChunked()
    valueLower = http_header_utils::toLowerCopy(value);
    if (valueLower.find("chunked") != std::string::npos) _isChunked = true;
  }
}

/**
 * @brief Valida headers obligatorios y conflictos.
 * @return true si los headers son válidos, false si hay error.
 */
bool HttpParser::validateHeaders() const {
  // Host es obligatorio en HTTP/1.1
  if (_request.getVersion() == HTTP_VERSION_1_1 &&
      _request.getHeader("host").empty())
    return false;

  // Si llegan ambos headers (Transfer-Encoding y Content-Length),
  // se ignora Content-Length y se usa chunked (comportamiento actual).

  // Límite de body: lo setea el Client con setMaxBodySize() antes de consume().
  if (_maxBodySize > 0 && _contentLength > _maxBodySize) return false;

  return true;
}

bool HttpParser::processHeaderLine(const std::string& line) {
  std::string key;
  std::string value;

  if (!splitHeaderLine(line, key, value)) return false;

  handleHeader(key, value);
  return true;
}

// PARSE HEADERS -------------------------------------------------------------
void HttpParser::parseHeaders() {
  while (true) {
    std::string line;
    if (!extractLine(line))
      return;  // No hay línea completa, esperamos al siguiente epoll()
    // Caso 1: Línea vacía -> Fin de headers
    if (line.empty() && !validateHeaders()) {
      _errorStatusCode = (_maxBodySize > 0 && _contentLength > _maxBodySize)
                             ? 413
                             : 400;
      _state = ERROR;
      return;
    }

    if (line.empty())  // fin de headers \r\n\r\n
    {
      if (_isChunked == true || _contentLength > 0)
        _state = PARSING_BODY;
      else
        _state = COMPLETE;
      return;
    }

    // Caso 2: Línea con datos -> Procesar
    if (!processHeaderLine(line)) {
      _errorStatusCode = 400;
      _state = ERROR;
      return;
    }
  }
}
