## CGI – Variables de entorno que prepara `CgiExecutor`

Este documento NO cambia el código de Carles, solo lo explica.

Archivo relevante: `src/cgi/CgiExecutor.cpp`, función:

```cpp
std::map< std::string, std::string >
CgiExecutor::prepareEnvironment(const HttpRequest& request,
                                const std::string& script_path);
```

Esta función construye el `envp` que recibe el script CGI.

---

### 1. Variables CGI / HTTP básicas

```cpp
env["GATEWAY_INTERFACE"] = "CGI/1.1";
env["SERVER_PROTOCOL"]   = "HTTP/1.1";
env["SERVER_SOFTWARE"]   = "Webserv/1.0";
env["REQUEST_METHOD"]    = methodToString(request.getMethod());
```

- **GATEWAY_INTERFACE**: versión del protocolo CGI.
- **SERVER_PROTOCOL**: versión HTTP que habla el servidor.
- **SERVER_SOFTWARE**: nombre de tu servidor.
- **REQUEST_METHOD**: `GET`, `POST`, `DELETE`, etc.

---

### 2. Rutas del script: `SCRIPT_FILENAME` y `SCRIPT_NAME`

```cpp
env["SCRIPT_FILENAME"] = script_path;

std::string uri = request.getPath();
if (!request.getQuery().empty()) {
    uri += "?";
    uri += request.getQuery();
}
size_t question_mark = uri.find('?');
std::string script_name =
    (question_mark != std::string::npos) ? uri.substr(0, question_mark) : uri;
env["SCRIPT_NAME"] = script_name;
```

- **SCRIPT_FILENAME**:
  - Ruta absoluta en el sistema de ficheros al script CGI.
  - Ej: `/Users/ana/webserv/www/cgi-bin/hello.py`

- **SCRIPT_NAME**:
  - La parte de la **URI** que apunta al script (sin query string).
  - Ej: si el cliente pide `/cgi-bin/hello.py?name=Ana`, entonces:
    - `SCRIPT_NAME=/cgi-bin/hello.py`

---

### 3. `QUERY_STRING`

```cpp
std::string query_string = "";
if (question_mark != std::string::npos && question_mark + 1 < uri.length()) {
    query_string = uri.substr(question_mark + 1);
}
env["QUERY_STRING"] = query_string;
```

- Todo lo que va **después del `?`** en la URL.
- Ej: `/cgi-bin/test.py?foo=42&bar=baz` → `QUERY_STRING=foo=42&bar=baz`
- Para un GET sin query, será la cadena vacía.

---

### 4. Información del body: `CONTENT_LENGTH` y `CONTENT_TYPE`

```cpp
std::ostringstream len;
len << request.getBody().size();
env["CONTENT_LENGTH"] = len.str();

std::string ct = request.getHeader("content-type");
if (!ct.empty())
    env["CONTENT_TYPE"] = ct;
```

- **CONTENT_LENGTH**:
  - Número de bytes del body de la petición.
  - Es lo que el CGI usa para saber cuántos bytes leer de `stdin` en un POST.

- **CONTENT_TYPE**:
  - Tipo del contenido, si el cliente envía el header `Content-Type`.
  - Ej: `text/plain`, `application/x-www-form-urlencoded`, `multipart/form-data`, etc.

---

### 5. Información de conexión: `REMOTE_ADDR` y `REQUEST_URI`

```cpp
env["REMOTE_ADDR"] = "127.0.0.1"; // TODO: Extract from socket
env["REQUEST_URI"] = uri;
```

- **REMOTE_ADDR**:
  - IP del cliente. De momento está fijada a `127.0.0.1` (localhost).
  - En una versión más avanzada se leería de la conexión real del socket.

- **REQUEST_URI**:
  - La URI completa tal y como llegó del cliente, con query incluida.
  - Ej: `/cgi-bin/test.py?foo=42`

---

### 6. Headers HTTP como variables `HTTP_*`

```cpp
const std::map< std::string, std::string >& headers = request.getHeaders();
for (std::map< std::string, std::string >::const_iterator it = headers.begin();
     it != headers.end(); ++it) {
    std::string key = it->first;
    std::string env_key = "HTTP_";
    for (size_t i = 0; i < key.length(); ++i) {
        char c = key[i];
        if (c == '-')
            env_key += '_';
        else
            env_key += toupper(c);
    }
    env[env_key] = it->second;
}
```

Conversión:

- Toma cada header HTTP, por ejemplo:
  - `User-Agent: curl/8.0`
  - `Accept-Language: es-ES`
- Lo transforma en:
  - `HTTP_USER_AGENT=curl/8.0`
  - `HTTP_ACCEPT_LANGUAGE=es-ES`

Reglas:

- Se añade el prefijo `HTTP_`.
- Los guiones `-` se convierten en `_`.
- Las letras se pasan a mayúsculas.

---

### 7. Resumen corto

Cuando se ejecuta un script CGI:

- `CgiExecutor::prepareEnvironment` construye un `std::map` con todas las variables.
- Luego se convierte ese mapa en `char** envp` para llamar a `execve`.
- El script CGI ve estas variables igual que vería las de un servidor real (Apache, Nginx, etc.).

Tu parte como autora del cliente:

- Asegurarte de que `HttpRequest` tiene:
  - Método correcto (`GET`, `POST`, etc.).
  - Path y query (`getPath()` y `getQuery()`).
  - Headers.
  - Body.

El resto (cómo se monta `envp`) ya lo abstrae `CgiExecutor`.

