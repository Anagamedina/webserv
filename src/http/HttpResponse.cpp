#include "HttpResponse.hpp"
#include "HttpHeaderUtils.hpp"

#include <sstream>

static std::string reasonPhraseForStatus(int code) {
    switch (code) {
    case HTTP_STATUS_OK:
        return "OK";
    case HTTP_STATUS_BAD_REQUEST:
        return "Bad Request";
    case HTTP_STATUS_NOT_FOUND:
        return "Not Found";
    case HTTP_STATUS_METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case HTTP_STATUS_INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
    default:
        return "Unknown";
    }
}

static std::string versionToString(HttpVersion version) {
    if (version == HTTP_VERSION_1_0)
        return "HTTP/1.0";
    if (version == HTTP_VERSION_1_1)
        return "HTTP/1.1";
    return "HTTP/1.1";
}

HttpResponse::HttpResponse() :
      _status(HTTP_STATUS_OK),
      _version(HTTP_VERSION_1_1),
      _headers(),
      _reasonPhrase(reasonPhraseForStatus(HTTP_STATUS_OK)),
      _body() {
}

HttpResponse::HttpResponse(const HttpResponse& other) :
      _status(other._status),
      _version(other._version),
      _headers(other._headers),
      _reasonPhrase(other._reasonPhrase),
      _body(other._body) {
}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        _status = other._status;
        _version = other._version;
        _headers = other._headers;
        _reasonPhrase = other._reasonPhrase;
        _body = other._body;
    }
    return *this;
}

HttpResponse::~HttpResponse() {
}

void HttpResponse::setStatusCode(int code) {
    _status = static_cast< HttpStatusCode >(code);
    _reasonPhrase = reasonPhraseForStatus(code);
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    _headers[http_header_utils::toLowerCopy(key)] = value;
}

void HttpResponse::setVersion(const std::string& version) {
    if (version == "HTTP/1.0")
        _version = HTTP_VERSION_1_0;
    else if (version == "HTTP/1.1")
        _version = HTTP_VERSION_1_1;
    else
        _version = HTTP_VERSION_UNKNOWN;
}

void HttpResponse::setReasonPhrase(const std::string& reason) {
    _reasonPhrase = reason;
}
// el reto es pegar la cabecera
// setters para binarios (imagenes)
void HttpResponse::setBody(const std::vector< char >& body) {
    _body = body;
}

void HttpResponse::setBody(const std::string& body) {
    _body.assign(body.begin(), body.end());
}

// SERIALIZE
//
std::vector< char > HttpResponse::serialize() const {
    std::stringstream buffer;

    // construir la primera linia y header
    buffer << versionToString(_version) << " " << _status << " " << _reasonPhrase << "\r\n";

    // iterar pior el mapa de headers, headers: key : value\r\n
    // no se declara _headers porque?
    for (HeaderMap::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        // evitar duplicar el header content-length
        if (http_header_utils::toLowerCopy(it->first) == "content-length")
            continue;
        buffer << it->first << ": " << it->second << "\r\n";
    }

    // TODO: agregar header Date cuando sea obligatorio.
    buffer << "Content-Length: " << _body.size() << "\r\n";

    buffer << "\r\n";

    // convertir la parte de texto a vector
    std::string headStr = buffer.str();
    std::vector< char > response(headStr.begin(), headStr.end());

    // insertar el cuerpo binario al final

    response.insert(response.end(), _body.begin(), _body.end());

    return (response);
}

void HttpResponse::setContentType(const std::string& filename) {
    std::string::size_type dotPos = filename.find_last_of('.');
    std::string ext;

    if (dotPos == std::string::npos)
        ext = "";
    else
        ext = http_header_utils::toLowerCopy(filename.substr(dotPos + 1));

    std::string contentType = "application/octet-stream";
    if (ext == "html" || ext == "htm")
        contentType = "text/html";
    else if (ext == "css")
        contentType = "text/css";
    else if (ext == "js")
        contentType = "application/javascript";
    else if (ext == "png")
        contentType = "image/png";
    else if (ext == "jpg" || ext == "jpeg")
        contentType = "image/jpeg";
    else if (ext == "gif")
        contentType = "image/gif";
    else if (ext == "svg")
        contentType = "image/svg+xml";
    else if (ext == "txt")
        contentType = "text/plain";

    setHeader("Content-Type", contentType);
}
