#include "HttpRequest.hpp"
#include <algorithm> //para convertir a mayúsculas transform
#include <cctype>    //para convertir a minúsculas

// ============================================================================
// CONSTRUCTOR Y DESTRUCTOR
// ============================================================================

// constructor por defecto
HttpRequest::HttpRequest() :
      _method(HTTP_METHOD_UNKNOWN),
      _version(HTTP_VERSION_UNKNOWN),
      _headers(),
      _status(HTTP_STATUS_PENDING),
      _path(),
      _query() {
}
// constructor de inicialización
HttpRequest::HttpRequest(const std::string& method, const std::string& version,
                         const HeaderMap& headers, const std::string& path,
                         const std::string& query, const std::vector< char >& body) :
      _method(HTTP_METHOD_UNKNOWN),
      _version(HTTP_VERSION_UNKNOWN),
      _headers(headers),
      _status(HTTP_STATUS_PENDING),
      _path(path),
      _query(query),
      _body(body) {
    setMethod(method);   // ← Convierte "GET" → HTTP_METHOD_GET
    setVersion(version); // ← Convierte "HTTP/1.1" → HTTP_VERSION_1_1
}

// constructor de copia
HttpRequest::HttpRequest(const HttpRequest& other) :
      _method(other._method),
      _version(other._version),
      _headers(other._headers),
      _status(other._status),
      _path(other._path),
      _query(other._query) {
}

// operador de asignación
HttpRequest& HttpRequest::operator=(const HttpRequest& other) {
    if (this != &other) {
        _method = other._method;
        _version = other._version;
        _headers = other._headers;
        _path = other._path;
        _query = other._query;
        _body = other._body;
        _status = other._status;
    }
    return *this;
}

HttpRequest::~HttpRequest() {
}

// ============================================================================
// SETTERS (usados por el parser)
// ============================================================================
/**
 * @brief Convierte el método HTTP a mayúsculas y lo asigna al atributo _method
 *
 * @param method
 */
void HttpRequest::setMethod(const std::string& method) {
    std::string upperMethod = method;
    // transform recibe 3 argumentos:
    // 1. El inicio del rango de caracteres a transformar
    // 2. El final del rango de caracteres a transformar
    // 3. El resultado de la transformación

    std::transform(upperMethod.begin(), upperMethod.end(), upperMethod.begin(), ::toupper);

    if (upperMethod == "GET")
        _method = HTTP_METHOD_GET;
    else if (upperMethod == "POST")
        _method = HTTP_METHOD_POST;
    else if (upperMethod == "DELETE")
        _method = HTTP_METHOD_DELETE;
    else
        _method = HTTP_METHOD_UNKNOWN;
}

void HttpRequest::setVersion(const std::string& version) {
    if (version == "HTTP/1.0")
        _version = HTTP_VERSION_1_0;
    else if (version == "HTTP/1.1")
        _version = HTTP_VERSION_1_1;
    else
        _version = HTTP_VERSION_UNKNOWN;
}

/**
 * @brief Agrega un header al mapa. La clave debe venir ya normalizada en minúsculas.
 *
 * @param key : nombre del header (ya en minúsculas)
 * @param value : contenido del header ej: "localhost:8080"
 * @note HTTP es case-insensitive, por eso el parser normaliza antes.
 */
void HttpRequest::addHeaders(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void HttpRequest::setPath(const std::string& path) {
    _path = path;
}

void HttpRequest::setQuery(const std::string& query) {
    _query = query;
}

// void HttpRequest::addBody(const std::vector<char>& chunk)
// {
//     _body.insert(_body.end(), chunk.begin(), chunk.end());
// }

void HttpRequest::addBody(std::string::const_iterator begin, std::string::const_iterator end) {
    if (begin == end)
        return;
    // vector.insert(donde_pegar, inicio_del_rango, fin_del_rango);
    _body.insert(_body.end(), begin, end);
}

// ============================================================================
// GETTERS (usados por la lógica de respuesta y CGI)
// ============================================================================

HttpMethod HttpRequest::getMethod() const {
    return _method;
}

HttpVersion HttpRequest::getVersion() const {
    return _version;
}

/**
 * @brief Obtiene un header específico de la petición
 *
 * @param key : es el nombre del header ej: "Host", "Content-Length", "Connection"
 * @return const std::string& : es el contenido del header ej: "localhost:8080" o "100000" o
 * "keep-alive"
 *
 */

const std::string& HttpRequest::getHeader(const std::string& key) const {
    // TODO: Optimización futura (cachear los resultados)

    std::string lowerKey;
    // static porque se crea una sola vez y se reutiliza en todas las llamadas a la función
    static const std::string empty = "";

    lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    HeaderMap::const_iterator it = _headers.find(lowerKey);
    if (it != _headers.end())
        return it->second; // value lo que nos interesa

    return empty;
}

/**
 * @brief Obtiene todos los headers de la petición
 *
 * @return const HttpRequest::HeaderMap& : es un mapa con todos los headers
 * y util para CGI que necesita iterar sobre todos los headers para variables de entorno.  Carles
 */
const HttpRequest::HeaderMap& HttpRequest::getHeaders() const {
    return _headers; // Devolver referencia constante al mapa completo
}

std::string HttpRequest::getPath() const {
    return _path;
}

std::string HttpRequest::getQuery() const {
    return _query;
}

std::vector< char > HttpRequest::getBody() const {
    return _body;
}

HttpStatus HttpRequest::getStatus() const {
    return _status;
}

// ============================================================================
// HELPERS
// ============================================================================

/**
 * @brief Limpia el request porque se puede reutilizar el objeto para otra petición
 * y se debe limpiar el estado del request para que no haya interferencias con la siguiente petición
 *
 * @note resetea el status code HTTP a PENDING
 */
void HttpRequest::clear() {
    _method = HTTP_METHOD_UNKNOWN;
    _version = HTTP_VERSION_UNKNOWN;
    _headers.clear();
    _path.clear();
    _query.clear();
    _body.clear();
    _status = HTTP_STATUS_PENDING; // resetea el status code HTTP a PENDING
}

// ============================================================================
// LÓGICA DE CONEXIÓN (keep-alive vs close)
// ============================================================================

/**
 * @brief Verifica si se debe cerrar la conexión con el cliente después de enviar la respuesta
 *
 * @note HTTP/1.1: Por defecto es keep-alive (persistente)
 * Solo cerramos si explícitamente dice "close"
 * HTTP/1.0: Por defecto se cierra después de cada respuesta
 * Solo mantenemos abierta si explícitamente dice "keep-alive"
 * HTTP_VERSION_UNKNOWN o versión no soportada: cerrar por seguridad
 */
bool HttpRequest::shouldCloseConnection() const {
    std::string connectionHeader;

    connectionHeader = getHeader("Connection");
    std::transform(connectionHeader.begin(), connectionHeader.end(), connectionHeader.begin(),
                   ::tolower);

    if (_version == HTTP_VERSION_1_1) {
        return (connectionHeader == "close");
    } else if (_version == HTTP_VERSION_1_0) {

        return (connectionHeader != "keep-alive");
    } else {
        // HTTP_VERSION_UNKNOWN o versión no soportada: cerrar por seguridad
        return true;
    }
}
