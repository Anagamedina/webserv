# Apuntes de repaso: HTTP, Processor y Client

> Resumen para entender el parseo HTTP, el RequestProcessor y el Client.

---

## 1. Visión general del flujo

```
[Socket] → recv() → bytes crudos
    ↓
[HttpParser] → consume(bytes) → HttpRequest (COMPLETE o ERROR)
    ↓
[Client] → buildResponse() → ¿CGI? → ClientCgi / RequestProcessor
    ↓
[RequestProcessor] → matching + validación + StaticPathHandler
    ↓
[HttpResponse] → serialize() → bytes → send()
```

---

## 2. Capa HTTP: HttpParser

### Estados del parser

| Estado | Significado |
|--------|-------------|
| `PARSING_START_LINE` | Leyendo `METHOD URI VERSION` |
| `PARSING_HEADERS` | Leyendo líneas `Key: Value` |
| `PARSING_BODY` | Leyendo body (Content-Length o chunked) |
| `COMPLETE` | Request lista |
| `ERROR` | Parseo inválido |

### Archivos

| Archivo | Responsabilidad |
|---------|-----------------|
| `HttpParser.cpp` | `consume()`, orquesta el flujo |
| `HttpParserStartLine.cpp` | Método, URI, query, versión |
| `HttpParserHeaders.cpp` | Headers, Content-Length, Transfer-Encoding chunked |
| `HttpParserBody.cpp` | Body fijo (Content-Length) o chunked |
| `HttpRequest.hpp/cpp` | Objeto resultado: method, path, query, headers, body |
| `HttpResponse.hpp/cpp` | Objeto respuesta: status, headers, body, `serialize()` |

### Body: dos modos

- **Content-Length:** Lee exactamente N bytes.
- **Transfer-Encoding: chunked:** Lee chunks en formato `tamaño_hex\r\ndatos\r\n`, hasta `0\r\n\r\n`.

---

## 3. Clase Client

Cada conexión TCP tiene un `Client`. Gestiona:

- Lectura del socket → alimentar al parser
- Construcción de la respuesta (CGI o estático)
- Escritura al socket (buffer + cola de respuestas)
- Sesión CGI (pipes, CgiProcess)

### Estados del Client

| Estado | Significado |
|--------|-------------|
| `STATE_IDLE` | Esperando o listo para nueva petición |
| `STATE_READING_HEADER` | Recibiendo datos |
| `STATE_READING_BODY` | CGI en curso, leyendo salida del hijo |
| `STATE_WRITING_RESPONSE` | Enviando respuesta |
| `STATE_CLOSED` | Conexión cerrada |

### Flujo en Client

```
handleRead():
  recv() → _parser.consume(data)
  si COMPLETE → handleCompleteRequest()
  si ERROR   → handleCompleteRequest() (respuesta 400)

handleCompleteRequest():
  buildResponse()
  si no CGI → serializar _response, enqueueResponse()
  si CGI    → startCgiIfNeeded() ya hizo el trabajo, respuesta vendrá por pipes

buildResponse():
  si startCgiIfNeeded() == true → return (CGI maneja la respuesta)
  si no → RequestProcessor::process(request, ..., _response)
```

---

## 4. RequestProcessor

Recibe `HttpRequest` y configura, y rellena `HttpResponse`.

### Pasos de process()

1. **Errores de parseo** → 400 Bad Request
2. **Seleccionar server** por puerto (`selectServerByPort`)
3. **Seleccionar location** por URI (`matchLocation`)
4. **Validar** método, body size, redirección
5. **Resolver path real** (`root` + URI)
6. **¿CGI?** → Si es CGI, se delega en Client (startCgiIfNeeded). Aquí puede devolver 501 si no está integrado.
7. **¿Estático?** → `handleStaticPath()`: archivo, directorio, upload, DELETE, autoindex

---

## 5. StaticPathHandler (handleStaticPath)

Decide qué tipo de recurso es y actúa:

| Caso | Condición | Acción |
|-----|-----------|--------|
| **Upload** | POST + `upload_store` en location | Guardar body en `upload_store/nombre` |
| **Directorio** | path es directorio | Buscar index, o autoindex, o 403 |
| **Archivo** | path es archivo | Leer y servir |
| **DELETE** | Método DELETE | `unlink()`, responder 204/200 |
| **No existe** | stat falla | 404 |

---

## 6. ResponseUtils y SessionUtils

- **fillBaseResponse():** Asigna status, Connection, Content-Type, body, y llama a `addSessionCookieIfNeeded()`.
- **addSessionCookieIfNeeded():** Añade `Set-Cookie: id=...` en respuestas 2xx si no hay sesión válida.
- **ErrorUtils / buildErrorResponse():** Construye respuestas de error (404, 405, 413, 500) con páginas personalizadas o HTML por defecto.

---

## 7. Integración CGI en Client (ClientCgi)

```
startCgiIfNeeded(request):
  - Matching server/location
  - ¿Es CGI por extensión o por config?
  - CgiExecutor::executeAsync() → CgiProcess*
  - registerCgiPipe(pipe_out, EPOLLIN)
  - registerCgiPipe(pipe_in, EPOLLOUT)
  - _state = STATE_READING_BODY

handleCgiPipe(pipe_fd, events):
  - Si EPOLLIN en pipe_out → leer stdout del hijo
  - Si EPOLLOUT en pipe_in → escribir body al stdin del hijo
  - Cuando proceso termina / headers+body listos → finalizeCgiResponse()
  - parseCgiHeaders(), setBody(), enqueueResponse()
```

---

## 8. HttpRequest: campos útiles

| Campo | Getter | Uso |
|-------|--------|-----|
| Método | `getMethod()` | GET, POST, DELETE |
| Path | `getPath()` | `/index.html` |
| Query | `getQuery()` | `a=1&b=2` |
| Headers | `getHeader("host")`, `getHeaders()` | Host, Cookie, Content-Type |
| Body | `getBody()` | vector<char> |
| Connection | `shouldCloseConnection()` | close vs keep-alive |

---

## 9. HttpResponse: uso típico

```cpp
response.setStatusCode(200);
response.setHeader("Content-Type", "text/html");
response.setBody(body);
// SessionUtils añade Set-Cookie si corresponde
std::vector<char> raw = response.serialize();
// raw = "HTTP/1.1 200 OK\r\nContent-Type: ...\r\n\r\n" + body
```

---

## 10. Diagrama de dependencias (conceptual)

```
Client
  ├─ HttpParser → HttpRequest
  ├─ RequestProcessor → HttpResponse, StaticPathHandler, ErrorUtils, ResponseUtils
  ├─ SessionUtils (vía ResponseUtils)
  ├─ CgiExecutor, CgiProcess (si es CGI)
  └─ ServerManager (para registerCgiPipe)
```

---

## 11. Checklist de repaso

- [ ] Entiendo los estados del HttpParser (start line → headers → body → complete)
- [ ] Sé qué es Content-Length vs chunked y dónde se parsea
- [ ] Conozco el flujo Client: handleRead → buildResponse → handleCompleteRequest
- [ ] Entiendo RequestProcessor: matching, validación, handleStaticPath
- [ ] Sé qué hace handleStaticPath (archivo, directorio, upload, DELETE)
- [ ] Conozco el flujo CGI: startCgiIfNeeded, handleCgiPipe, finalizeCgiResponse
