#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <map>
#include <string>
#include <vector>

#include "HttpRequest.hpp"  // para reutilizar HttpVersion

// Códigos de estado mínimos para empezar.
enum HttpStatusCode {
  HTTP_STATUS_OK = 200,
  HTTP_STATUS_CREATED = 201,
  HTTP_STATUS_BAD_REQUEST = 400,
  HTTP_STATUS_FORBIDDEN = 403,
  HTTP_STATUS_NOT_FOUND = 404,
  HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
  HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE = 413,
  HTTP_STATUS_INTERNAL_SERVER_ERROR = 500
};

// Representa una respuesta HTTP que se enviará al cliente.
class HttpResponse {
 private:
  typedef std::map<std::string, std::string> HeaderMap;
  HttpStatusCode _status;
  HttpVersion _version;
  HeaderMap _headers;
  std::string _reasonPhrase;
  std::vector<char> _body;

 public:
  HttpResponse();
  HttpResponse(const HttpResponse& other);
  HttpResponse& operator=(const HttpResponse& other);
  ~HttpResponse();

  // SETTERS

  void setStatusCode(int code);
  void setBody(const std::vector<char>& body);
  void setHeader(const std::string& key, const std::string& value);
  void setVersion(const std::string& version);
  // La «razón» (reason phrase) en las respuestas HTTP es un texto breve y
  // legible por humanos que acompaña al código de estado n     //umérico (ej.
  // 200)
  void setReasonPhrase(const std::string& reason);
  // para cuando envias HTML simple o texto
  void setBody(const std::string& body);

  // SERIALIZE
  // lo hago vector para que poder enviarlo bien a send() sin que corte si
  // hay un byte nulo en medio de una imagen.
  std::vector<char> serialize() const;

  // HELPERS
  // segun la extension del archivo
  void setContentType(const std::string& filename);
  // comprobar si ya existe un header (se usa para no sobreescribir
  // Content-Type)
  bool hasHeader(const std::string& key) const;
};

#endif  // HTTP_RESPONSE_HPP
