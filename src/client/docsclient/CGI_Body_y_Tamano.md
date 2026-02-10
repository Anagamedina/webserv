## CGI – Body, métodos y límite de tamaño

Este documento explica, paso a paso y en modo “principiante”, cómo maneja tu servidor:

- Peticiones **GET** y **POST** hacia CGI.
- Envío del **body completo** al script CGI.
- El límite de tamaño `client_max_body_size` **antes** de lanzar el CGI.

---

### 1. Dónde se toca este tema en el código

- `ClientCgi.cpp`
  - Función: `bool Client::startCgiIfNeeded(const HttpRequest& request)`
  - Decide si una petición va a CGI o no.
  - Valida el tamaño del body antes de lanzar el CGI.

- `CgiExecutor.cpp`
  - Clase: `CgiExecutor`
  - Función: `CgiProcess* executeAsync(const HttpRequest& request, const std::string& script_path, const std::string& interpreter_path)`
  - Convierte el body del `HttpRequest` en `std::string` y se lo pasa al `CgiProcess`.

- `CgiProcess.hpp / CgiProcess.cpp`
  - Clase: `CgiProcess`
  - Guarda:
    - Todo el body de la petición (`request_body_`).
    - Cuántos bytes ya se han escrito al CGI (`body_bytes_written_`).
  - Tiene helpers para escribir el body en varias tandas.

---

### 2. GET sin body vs POST con body

#### 2.1. Qué body ve el CGI

En `CgiExecutor::executeAsync`:

- Se convierte el body del `HttpRequest` a `std::string`:

```cpp
std::string body = bodyToString(request.getBody());
```

- Ese `body` se pasa al constructor de `CgiProcess`:

```cpp
CgiProcess* proc = new CgiProcess(
    script_path,
    interpreter_path,
    pipe_in[1],      // donde el padre escribe el body
    pipe_out[0],     // donde el padre lee la respuesta
    pid,
    5,               // timeout en segundos
    body             // body completo de la petición
);
```

#### 2.2. Qué pasa si es GET

- En una petición **GET** normal, `request.getBody()` está vacío.
- Entonces `body` será una cadena vacía (`""`).
- El CGI se lanza igual, pero no se envía contenido por stdin.
- El script CGI puede seguir usando:
  - `REQUEST_METHOD=GET`
  - `QUERY_STRING=...`
  para saber qué hacer.

#### 2.3. Qué pasa si es POST

- En una petición **POST**, `request.getBody()` contiene todos los bytes del cuerpo.
- Ese body se guarda en `CgiProcess::request_body_`.
- Luego, en `Client::handleCgiPipe(...)`, el body se envía poco a poco al CGI.

Código relevante en `ClientCgi.cpp`:

```cpp
if (pipe_fd == _cgiProcess->getPipeIn() && (events & EPOLLOUT)) {
    const std::string& body = _cgiProcess->getRequestBody();
    size_t offset = _cgiProcess->getBodyBytesWritten();
    if (offset < body.size()) {
        ssize_t written = write(pipe_fd, body.c_str() + offset, body.size() - offset);
        if (written > 0) {
            _cgiProcess->advanceBodyBytesWritten(static_cast< size_t >(written));
            _lastActivity = std::time(0);
        }
    }
    if (_cgiProcess->isRequestBodySent()) {
        _cgiProcess->closePipeIn();
        _serverManager->unregisterCgiPipe(pipe_fd);
    }
    return;
}
```

Traducción a lenguaje humano:

- Mientras quede body por escribir (`offset < body.size()`), se hace un `write` al pipe.
- Se actualiza cuánto llevamos escrito (`advanceBodyBytesWritten`).
- Cuando se ha escrito todo (`isRequestBodySent()`), cerramos el `stdin` del CGI (`closePipeIn`) y desregistramos ese pipe de `epoll`.

Así, **POST con body se envía completo** al script CGI.  
GET simplemente tiene un body vacío y no se escribe nada.

---

### 3. Límite `client_max_body_size` antes de lanzar CGI

#### 3.1. Dónde se valida

En `ClientCgi.cpp`, dentro de `Client::startCgiIfNeeded`:

```cpp
const ServerConfig* server = selectServerByPort(_listenPort, _configs);
if (server == 0)
    return false;
const LocationConfig* location = matchLocation(*server, request.getPath());
if (location == 0)
    return false;

// 1) Validar tamaño del body antes de lanzar CGI (proteccion simple).
//    Usamos el max_body_size del server. Si se supera, devolvemos 413.
if (server && request.getBody().size() > server->getMaxBodySize()) {
    buildErrorResponse(_response, request, HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE, true, server);
    return true;
}
```

Qué significa:

- Se obtiene el `ServerConfig` que corresponde al puerto actual (`selectServerByPort`).
- Se hace `matchLocation` para saber qué `location` aplica.
- **Antes** de resolver la ruta del script y lanzar el CGI:
  - Se mira el tamaño del body: `request.getBody().size()`.
  - Se compara con `server->getMaxBodySize()` → esto viene de `client_max_body_size` en la config.
  - Si el body es más grande:
    - NO se lanza el CGI.
    - Se construye una respuesta de error 413:

```cpp
buildErrorResponse(_response, request, HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE, true, server);
return true;
```

#### 3.2. Código 413 en HttpResponse

Para que el 413 sea claro, se añadió al enum y al texto:

```cpp
// HttpResponse.hpp
enum HttpStatusCode {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE = 413,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500
};
```

```cpp
// HttpResponse.cpp
static std::string reasonPhraseForStatus(int code) {
    switch (code) {
    case HTTP_STATUS_OK:                    return "OK";
    case HTTP_STATUS_BAD_REQUEST:           return "Bad Request";
    case HTTP_STATUS_NOT_FOUND:             return "Not Found";
    case HTTP_STATUS_METHOD_NOT_ALLOWED:    return "Method Not Allowed";
    case HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE:
        return "Request Entity Too Large";
    case HTTP_STATUS_INTERNAL_SERVER_ERROR: return "Internal Server Error";
    default:                                return "Unknown";
    }
}
```

Resultado:

- Si un cliente manda un body más grande que `client_max_body_size`,  
  el servidor responde con:

```http
HTTP/1.1 413 Request Entity Too Large
...
```

Y **no** lanza el proceso CGI.

---

### 4. Resumen para recordar

1. **GET hacia CGI**:
   - Body vacío.
   - El CGI se basa en `REQUEST_METHOD=GET` y `QUERY_STRING`.

2. **POST hacia CGI**:
   - Body completo se guarda en `CgiProcess::request_body_`.
   - El `Client` lo envía poco a poco por el pipe de entrada al CGI.

3. **Límite de tamaño (`client_max_body_size`)**:
   - Se comprueba en `startCgiIfNeeded` **antes** de crear el CGI.
   - Si se supera, se devuelve 413 y se termina.

Con esto tienes la parte de métodos y cuerpos para CGI organizada y explicada de forma sencilla.

