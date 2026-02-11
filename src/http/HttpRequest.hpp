#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

enum HttpMethod {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
  HTTP_METHOD_DELETE,
  HTTP_METHOD_UNKNOWN
};

enum HttpStatus {
  HTTP_STATUS_PENDING,
  HTTP_STATUS_PARSING_HEADERS,
  HTTP_STATUS_PARSING_BODY,
  HTTP_STATUS_SENDING_RESPONSE,
  HTTP_STATUS_CLOSED,
  HTTP_STATUS_ERROR
};

enum HttpVersion { HTTP_VERSION_1_0, HTTP_VERSION_1_1, HTTP_VERSION_UNKNOWN };

class HttpRequest {
 private:
  typedef std::map<std::string, std::string> HeaderMap;

  HttpMethod _method;
  HttpVersion _version;
  HeaderMap _headers;
  HttpStatus _status;
  std::string _path;   // URL de la petición ej: "/images/logo.png"
  std::string _query;  // Query string de la petición ej: "?name=John&age=30"
  std::vector<char> _body;  // vector para soportar binarios y texto grande (ej:
                            // videos, imagenes, etc.)
 public:
  // constructors
  HttpRequest();
  HttpRequest(const HttpRequest& other);
  HttpRequest(const std::string& method, const std::string& version,
              const HeaderMap& headers, const std::string& path,
              const std::string& query, const std::vector<char>& body);
  ~HttpRequest();
  HttpRequest& operator=(const HttpRequest& other);

  // setters
  void setMethod(const std::string& method);
  void setVersion(const std::string& version);
  void addHeaders(const std::string& key, const std::string& value);
  void setPath(const std::string& path);
  void setQuery(const std::string& query);
  // void addBody(const std::vector<char>& chunk);
  void addBody(std::string::const_iterator begin,
               std::string::const_iterator end);
  void setStatus(HttpStatus status);

  // getters
  HttpMethod getMethod() const;
  HttpVersion getVersion() const;
  HttpStatus getStatus() const;
  // getters para headers
  const std::string& getHeader(const std::string& key) const;
  const HeaderMap& getHeaders() const;
  // getters para path y query
  std::string getPath() const;
  std::string getQuery() const;
  std::vector<char> getBody() const;

  // clear
  void clear();
  // shouldCloseConnection
  bool shouldCloseConnection() const;
  // Expect: 100-continue (cliente espera confirmación antes de enviar body grande)
  bool hasExpect100Continue() const;
};

#endif  // HTTP_REQUEST_HPP
