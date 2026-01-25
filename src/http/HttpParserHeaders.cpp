#include "HttpParser.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>



/**
 * @brief Funciones auxiliares para el parser de headers
 * 
 * @note son funciones auxiliares para el parser de headers
 * y se definen en el namespace para que no interfieran con el resto del codigo
 * ejemplo de value: "   Hello, World!   "
 * trimSpaces devuelve "Hello, World!"
 * ejemplo de value: "   Hello, World!   "
 * trimSpaces devuelve "Hello, World!"
 */
namespace http_parser_headers_helpers {

std::string trimSpaces(const std::string &value)
{
    std::string::size_type start;
    std::string::size_type end;
    //busca la primera posicion no vacia de la cadena, ignorando los espacios y tabs al principio y al final de la cadena 
    start = value.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    end = value.find_last_not_of(" \t");
    return value.substr(start, end - start + 1);
}

std::string toLowerCopy(const std::string &value)
{
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

} // namespace http_parser_headers_helpers


/**
 * @brief Alias para el namespace http_parser_headers_helpers
 * 
 * @note es un alias para el namespace http_parser_headers_helpers
 * y se define en el namespace para que no interfieran con el resto del codigo
 */
namespace headers_helpers = http_parser_headers_helpers;

/**
 * @brief Divide una linea de header en key y value
 * 
 */
bool HttpParser::splitHeaderLine(const std::string &line,
                                 std::string &key,
                                 std::string &value)
{
    std::string::size_type colon; 
    std::string rawValue; 
       
    colon = line.find(':');
    if (colon == std::string::npos)
        return false;

    key = line.substr(0, colon);
    rawValue = line.substr(colon + 1);
    value = headers_helpers::trimSpaces(rawValue);
    return true;
}


/**
* se encarga de manejar los headers de la peticion y 
*   añadirlos al mapa de headers del request ademas de 
*   manejar el content-length y el transfer-encoding activando 
*   el la flag de chunk si es necesario  
 * @param key: La key del header
 * @param value: El valor del header
 */
void HttpParser::handleHeader(const std::string &key, const std::string &value)
{
    std::string keyLower;
    std::string valueLower;

    // Normalizamos una sola vez y usamos la misma clave para:
    // 1) guardar en el mapa
    // 2) comparar
    keyLower = headers_helpers::toLowerCopy(key);
    _request.addHeaders(keyLower, value);
    if (keyLower == "content-length")
    {
        //convierte el valor a un numero entero
        //cuántos bytes exactos debe esperar antes de marcar la petición como COMPLETE.
        _contentLength = std::strtoul(value.c_str(), 0, 10);
        return;
    }

    if (keyLower == "transfer-encoding")
    {
        //Esto hará que tu parser ignore el Content-Length y use la lógica 
        //de los "vagones" (hexadecimales) que programaste en parseBodyChunked()
        valueLower = headers_helpers::toLowerCopy(value);
        if (valueLower.find("chunked") != std::string::npos)
            _isChunked = true;
    }
}



/**
 * @brief Valida headers obligatorios y conflictos.
 * @return true si los headers son válidos, false si hay error.
 */
bool HttpParser::validateHeaders() const
{
    // Host es obligatorio en HTTP/1.1
    // if (_request.getVersion() == HTTP_VERSION_1_1)
    // {
    //     if (_request.getHeader("host").empty())
    //         return false;
    // }
    if (_request.getVersion() == HTTP_VERSION_1_1 &&
        _request.getHeader("host").empty())
        return false;

    // Si llegan ambos headers (Transfer-Encoding y Content-Length),
    // se ignora Content-Length y se usa chunked (comportamiento actual).

    // Límite de body (si se configuró)
    // if (_maxBodySize > 0 && _contentLength > _maxBodySize)
    //     return false;

    return true;
}



bool HttpParser::processHeaderLine(const std::string &line)
{
    std::string key;
    std::string value;

    if (!splitHeaderLine(line, key, value))
        return false;

    handleHeader(key, value);
    return true;
}

// PARSE HEADERS -------------------------------------------------------------
void HttpParser::parseHeaders()
{
    while (true)
    {
        std::string line;
        if (!extractLine(line))
            return;// No hay línea completa, esperamos al siguiente epoll()
//// Caso 1: Línea vacía -> Fin de headers
        if (line.empty()) //fin de headers \r\n\r\n
        {
            // Forma anidada (original):
            // if (!validateHeaders()) { _state = ERROR; return; }

            // Forma más plana (con &&):
            if (line.empty() && !validateHeaders())
            {
                _state = ERROR;
                return;
            }

            if (_isChunked == true || _contentLength > 0)
                _state = PARSING_BODY;
            else
                _state = COMPLETE;
            return;
        }

        // Caso 2: Línea con datos -> Procesar
        if (!processHeaderLine(line))
        {
            _state = ERROR;
            return;
        }
    }
}
