## LaserWeb – High Level Flow (for draw.io)

This file is designed so you can recreate the diagrams manually in draw.io.  
It is split into three blocks:

- HTTP / Client (Ana – The Architect)
- epoll + CGI (Carles – The Executor)
- Config + Locations (Daru – The Operator)

For cada bloque te indico:
- **Nodes**: cajas que puedes crear en draw.io
- **Edges**: flechas entre nodos con etiquetas sugeridas
- Un **ASCII sketch** opcional para visualizar proporciones.

---

## 1. HTTP / Client – Request Flow (Ana)

### Nodes

1. `Browser / Client`
2. `TcpListener`  
3. `Client`  
4. `HttpParser`  
5. `Request` (resultado del parser)  
6. `RequestProcessor`  
7. `Static Handler`  
8. `CGI Executor` (este conecta con el diagrama de Carles)  
9. `HttpResponse`  
10. `Client (write socket)`  

### Edges (flechas)

- `Browser / Client` → `TcpListener`  
  - Label: **TCP CONNECT (port 8080)**

- `TcpListener` → `Client`  
  - Label: **accept() new fd**

- `Client` → `HttpParser`  
  - Label: **incoming bytes**

- `HttpParser` → `Request`  
  - Label: **HTTP/1.1 parsed**

- `Request` → `RequestProcessor`  
  - Label: **process(request)**

- `RequestProcessor` → `Static Handler`  
  - Label: **if static file / autoindex**

- `RequestProcessor` → `CGI Executor`  
  - Label: **if CGI route**

- `Static Handler` → `HttpResponse`  
  - Label: **build response (status + headers + body)**

- `CGI Executor` → `HttpResponse`  
  - Label: **CGI output packaged as HTTP**

- `HttpResponse` → `Client (write socket)`  
  - Label: **send()**


### ASCII sketch (orientativo)

```text
Browser/Client  -->  TcpListener  -->  Client  -->  HttpParser  -->  Request
                                                              |
                                                              v
                                                       RequestProcessor
                                                        /          \
                                                       v            v
                                               Static Handler    CGI Executor
                                                   |                 |
                                                   v                 v
                                                HttpResponse  <-------
                                                     |
                                                     v
                                            Client (write socket)
```

---

## 2. epoll + CGI – Async Execution (Carles)

### Nodes

1. `ServerManager`  
2. `EpollWrapper`  
3. `Listen Socket fd`  
4. `Client fd`  
5. `CGI stdin pipe fd`  
6. `CGI stdout pipe fd`  
7. `CgiExecutor`  
8. `fork()`  
9. `Child Process (CGI script)`  
10. `CgiProcess`  
11. `Client (handleCgiPipe)`  

### Edges

- `ServerManager` → `EpollWrapper`  
  - Label: **epoll_create + epoll_ctl**

- `Listen Socket fd` → `EpollWrapper`  
  - Label: **register (EPOLLIN)**

- `Client fd` → `EpollWrapper`  
  - Label: **register (EPOLLIN | EPOLLOUT as needed)**

- `EpollWrapper` → `ServerManager`  
  - Label: **epoll_wait(events)**

- `ServerManager` → `Client`  
  - Label: **if event on client fd → handleRead/handleWrite**

- `RequestProcessor` → `CgiExecutor`  
  - Label: **needs CGI**

- `CgiExecutor` → `fork()`  
  - Label: **create CGI process**

- `fork()` → `Child Process (CGI script)`  
  - Label: **execve() with env + stdin/stdout pipes**

- `CgiExecutor` → `CgiProcess`  
  - Label: **track pid + pipes + state**

- `CGI stdout pipe fd` → `EpollWrapper`  
  - Label: **register for EPOLLIN**

- `EpollWrapper` → `CgiProcess`  
  - Label: **data ready on stdout pipe**

- `CgiProcess` → `Client (handleCgiPipe)`  
  - Label: **appendResponseData()**

- `Client (handleCgiPipe)` → `HttpResponse`  
  - Label: **build final response from CGI headers + body**


### ASCII sketch

```text
          +--------------------+
          |   ServerManager    |
          +---------+----------+
                    |
                    v
             +--------------+
             | EpollWrapper |
             +--------------+
        /          |             \
       v           v              v
Listen fd     Client fd     CGI stdout fd
 (EPOLLIN)   (EPOLLIN/OUT)    (EPOLLIN)


RequestProcessor --needs CGI--> CgiExecutor --fork--> Child CGI (execve)
                                       |
                                       v
                                  CgiProcess
                                       |
                         CGI stdout pipe readable (epoll)
                                       |
                                       v
                             Client.handleCgiPipe()
                                       |
                                       v
                                 HttpResponse
```

---

## 3. Config + Locations – Routing Map (Daru)

### Nodes

1. `Config file (webserv.conf)`  
2. `ConfigParser`  
3. `ServerConfig[]`  
4. `ServerConfig` (single virtual host)  
5. `LocationConfig[]`  
6. `Location /`  
7. `Location /cgi-bin`  
8. `Location /uploads`  
9. `RequestProcessor`  
10. `Matched ServerConfig + LocationConfig`  

### Edges

- `Config file` → `ConfigParser`  
  - Label: **read + tokenize**

- `ConfigParser` → `ServerConfig[]`  
  - Label: **parse server blocks**

- `ServerConfig` → `LocationConfig[]`  
  - Label: **parse nested locations**

- `Request` → `RequestProcessor`  
  - Label: **host + path**

- `RequestProcessor` → `ServerConfig[]`  
  - Label: **select server by listen/host**

- `ServerConfig` → `LocationConfig[]`  
  - Label: **best-match prefix**

- `Location /` → `Matched LocationConfig` (ejemplo de ruta por defecto)  
  - Label: **root static**

- `Location /cgi-bin` → `Matched LocationConfig`  
  - Label: **CGI handler**

- `Location /uploads` → `Matched LocationConfig`  
  - Label: **upload_store**

- `Matched ServerConfig + LocationConfig` → `Static Handler / CGI Executor`  
  - Label: **apply rules (methods, max_body, etc.)**


### ASCII sketch

```text
webserv.conf
    |
    v
ConfigParser
    |
    v
ServerConfig[] ----------------------+
    |                                |
    v                                |
LocationConfig[] (per server)        |
                                     |
HTTP Request (host, path)            |
    |                                |
    v                                |
RequestProcessor                     |
    |                                |
    v                                |
Matched ServerConfig + LocationConfig+
    |
    +--> Static file / autoindex
    |
    +--> CGI (via CgiExecutor)
    |
    +--> Upload (write to upload_store)
```

---

Con estos tres bloques puedes montar en draw.io:

- Un diagrama grande con las tres áreas (`CLIENT/HTTP`, `EPOLL/CGI`, `CONFIG/LOCATIONS`) coloreadas distinto.
- O tres páginas separadas, una para cada miembro del equipo (Ana, Carles, Daru), usando los nodos y flechas descritos arriba.

---

## 4. HttpParser State Machine (start-line, headers, body)

Este bloque es para dibujar la **máquina de estados del parser HTTP** y cómo usa el buffer de entrada.

### States (nodes)

1. `PARSING_START_LINE`  
2. `PARSING_HEADERS`  
3. `PARSING_BODY`  
4. `COMPLETE`  
5. `ERROR`  

### Transitions (edges)
Las transiciones asumen que **el `Client` llama a `HttpParser::consume()` directamente con los bytes leídos del socket** (no hay `inBuffer` propio).

- `PARSING_START_LINE` → `PARSING_HEADERS`  
  - Condition: **valid start-line**  
  - Error: **else → ERROR**

- `PARSING_HEADERS` → `PARSING_HEADERS`  
  - Label: **consume header line (until CRLF)**  
  - Action: **append header; check size limits**

- `PARSING_HEADERS` → `PARSING_BODY`  
  - Condition: **found empty line (CRLF CRLF) AND `Content-Length` or `Transfer-Encoding` present**

- `PARSING_HEADERS` → `COMPLETE`  
  - Condition: **no body expected (e.g. GET without body)**

- `PARSING_HEADERS` → `ERROR`  
  - Condition: **invalid header / too big / malformed**

- `PARSING_BODY` → `PARSING_BODY`  
  - Label: **append to body buffer**  
  - Condition: **body not fully read**

- `PARSING_BODY` → `COMPLETE`  
  - Condition: **body fully read (by length or chunked FSM)**

- Any state → `ERROR`  
  - Condition: **parser invariant broken, invalid syntax, size exceeded**

### ASCII sketch

```text
            +----------------------+
            |  PARSING_START_LINE  |
            +----------+-----------+
                       |  ok
                       v
            +----------------------+
            |   PARSING_HEADERS    |
            +----+-----------+-----+
                 |           |
        no-body  |           |  has body
                 v           v
            +--------+   +------------------+
            |COMPLETE|   |   PARSING_BODY   |
            +--------+   +--------+---------+
                                   |
                          more body|  body complete
                                   v
                             +-----------+
                             | COMPLETE  |
                             +-----------+

Any invalid input → ERROR
```

---

## 5. Client State Machine (connection lifecycle)

Máquina de estados simplificada del objeto `Client` (conexión TCP).

### States (nodes)

1. `IDLE` – recién creado, sin datos.  
2. `READING_HEADERS` – leyendo start-line + headers.  
3. `READING_BODY` – leyendo body (si lo hay).  
4. `PROCESSING` – construyendo respuesta / lanzando CGI.  
5. `WRITING_RESPONSE` – enviando bytes al socket.  
6. `WAITING_CGI` – esperando salida de CGI (opcional, si tu implementación lo separa).  
7. `CLOSED` – conexión terminada.  

### Transitions (edges)

- `IDLE` → `READING_HEADERS`  
  - Trigger: **epoll says fd readable; recv() > 0**

- `READING_HEADERS` → `READING_HEADERS`  
  - Condition: **parser still in PARSING_HEADERS**

- `READING_HEADERS` → `READING_BODY`  
  - Condition: **headers complete and body expected**

- `READING_HEADERS` → `PROCESSING`  
  - Condition: **headers complete and no body**

- `READING_BODY` → `READING_BODY`  
  - Condition: **body not fully received**

- `READING_BODY` → `PROCESSING`  
  - Condition: **body complete**

- `PROCESSING` → `WAITING_CGI`  
  - Condition: **RequestProcessor says: use CGI**

- `PROCESSING` → `WRITING_RESPONSE`  
  - Condition: **static/autoindex/error built synchronously**

- `WAITING_CGI` → `WRITING_RESPONSE`  
  - Trigger: **CGI output complete (CgiProcess ready)**

- `WRITING_RESPONSE` → `WRITING_RESPONSE`  
  - Condition: **still bytes pending in outBuffer**

- `WRITING_RESPONSE` → `CLOSED`  
  - Condition: **all bytes sent AND connection: close or error**

- Any state (on fatal error / timeout) → `CLOSED`

### ASCII sketch

```text
          +------+
          | IDLE |
          +--+---+
             |
             v
   +-------------------+
   | READING_HEADERS   |
   +----+---------+----+
        |         |
        |body?    |no body
        v         v
+---------------------+      +-----------+
|    READING_BODY     | ---> |PROCESSING |
+----------+----------+      +-----+-----+
           |                 /      \
           |done body       /        \ CGI
           v               v          v
                        +--------+  +-----------+
                        |WRITING |  |WAITING_CGI|
                        |RESPONSE|  +-----+-----+
                        +---+----+        |
                            |             | CGI done
                            +-------------+
                                     |
                                     v
                                  +------+
                                  |CLOSED|
                                  +------+
```

---

## 6. Chunked Body Parsing State Machine

Esta es la sub-máquina de estados cuando el body lleva `Transfer-Encoding: chunked`.

### States (nodes)

1. `CHUNK_SIZE` – leyendo la línea con el tamaño del chunk en hex.  
2. `CHUNK_DATA` – leyendo exactamente `size` bytes de datos.  
3. `CHUNK_CRLF` – consumiendo el `\r\n` que sigue a los datos del chunk.  
4. `CHUNK_TRAILER` – leyendo headers de trailer (opcional) después de tamaño `0`.  
5. `CHUNK_COMPLETE` – se ha terminado el body.  
6. `CHUNK_ERROR` – error en formato o tamaño.  

### Transitions (edges)

- `CHUNK_SIZE` → `CHUNK_SIZE`  
  - Condition: **aún no se ha leído la línea completa (`\r\n`)**

- `CHUNK_SIZE` → `CHUNK_DATA`  
  - Condition: **línea completa; size > 0 (hex válido)**  
  - Action: **guardar `current_chunk_size`**

- `CHUNK_SIZE` → `CHUNK_TRAILER`  
  - Condition: **línea completa; size == 0**

- `CHUNK_SIZE` → `CHUNK_ERROR`  
  - Condition: **valor hex inválido o tamaño > límite**

- `CHUNK_DATA` → `CHUNK_DATA`  
  - Condition: **bytes leídos < current_chunk_size**

- `CHUNK_DATA` → `CHUNK_CRLF`  
  - Condition: **se han leído exactamente `current_chunk_size` bytes**

- `CHUNK_CRLF` → `CHUNK_SIZE`  
  - Condition: **se ha consumido el `\r\n` tras el chunk**

- `CHUNK_TRAILER` → `CHUNK_TRAILER`  
  - Condition: **leyendo líneas extra (trailers)**

- `CHUNK_TRAILER` → `CHUNK_COMPLETE`  
  - Condition: **línea vacía (fin de trailers)**

- Any state on error → `CHUNK_ERROR`

### ASCII sketch

```text
        +-------------+
        | CHUNK_SIZE  |
        +------+------+ 
               |  size>0
               v
        +-------------+
        | CHUNK_DATA  |
        +------+------+ 
               | read size bytes
               v
        +-------------+
        | CHUNK_CRLF  |
        +------+------+ 
               | got \r\n
               v
        +-------------+
        | CHUNK_SIZE  |  (next chunk)
        +-------------+

If size == 0 (in CHUNK_SIZE):
   CHUNK_SIZE --> CHUNK_TRAILER --> CHUNK_COMPLETE

Any bad format --> CHUNK_ERROR
```

---

## 7. End-to-end Request → Response (bytes → buffer → parser → processor → CGI/static → send)

Este diagrama junta lo que acabas de describir de forma narrativa:  
desde que el cliente manda bytes crudos por la red, hasta que se envían con `send()` cuando epoll marca el socket como listo para escritura.

### Nodes (cajas sugeridas)

1. `Browser / HTTP Client`  
2. `Kernel TCP Stack`  
3. `Listen Socket (fd)`  
4. `Client Socket (fd)`  
5. `Client::inBuffer (raw bytes)`  
6. `HttpParser::buffer`  
7. `HttpRequest`  
8. `RequestProcessor`  
9. `Static Handler`  
10. `CGI Executor`  
11. `Filesystem (www/...)`  
12. `CGI Scripts (www/cgi-bin/...)`  
13. `HttpResponse`  
14. `Client::outBuffer`  
15. `send()` / `epoll(EPOLLOUT)`  

### Flujo (edges, paso a paso)

1. **Cliente envía petición**
   - `Browser / HTTP Client` → `Kernel TCP Stack`  
     - Label: **TCP connect + HTTP/1.1 request bytes**

2. **Servidor acepta conexión**
   - `Kernel TCP Stack` → `Listen Socket (fd)`  
     - Label: **incoming connection**
   - `Listen Socket (fd)` → `Client Socket (fd)`  
     - Label: **accept() → new fd**

3. **epoll indica que hay datos para leer**
   - `Client Socket (fd)` → `Client::inBuffer`  
     - Label: **recv() → append raw bytes**

4. **Client entrega bytes al parser**
   - `Client::inBuffer` → `HttpParser::buffer`  
     - Label: **feed parser with available data**

5. **Parser construye la HttpRequest**
   - `HttpParser::buffer` → `HttpRequest`  
     - Label: **parse start-line + headers + (body)**  
   - Condición: **cuando el estado del parser llega a COMPLETE**

6. **Processor decide qué hacer con la request**
   - `HttpRequest` → `RequestProcessor`  
     - Label: **process(request, config)**
   - `RequestProcessor` → `Static Handler`  
     - Label: **if static route / autoindex / upload**
   - `RequestProcessor` → `CGI Executor`  
     - Label: **if CGI route (.py, .sh, etc.)**

7. **Acceso a disco o CGI**
   - `Static Handler` → `Filesystem (www/...)`  
     - Label: **open/read file / autoindex / write upload**
   - `CGI Executor` → `CGI Scripts (www/cgi-bin/...)`  
     - Label: **fork + execve script with env + stdin**

5. **Se construye la respuesta HTTP**
   - `Static Handler` → `HttpResponse`  
     - Label: **status + headers + body (file/autoindex/error)**
   - `CGI Executor` → `HttpResponse`  
     - Label: **wrap CGI stdout as HTTP response**

6. **La respuesta se encola para enviar**
   - `HttpResponse` → `Client::_outBuffer`  
     - Label: **serialize response bytes**

7. **epoll indica que el socket está listo para escritura**
    - `Client::_outBuffer` → `send()`  
      - Label: **on EPOLLOUT: write as much as possible**
    - `send()` → `Browser / HTTP Client`  
      - Label: **HTTP Response over TCP**

### ASCII sketch global

```text
Browser/HTTP Client
    |
    v
Kernel TCP Stack
    |
    v
Listen Socket (fd)
    |
    v  accept()
Client (fd = client_fd)
    |
    v  recv() when EPOLLIN
HttpParser::_buffer  --(state machine)-->  HttpRequest
                                        |
                                        v
                               RequestProcessor
                                /          \
                               v            v
                        Static Handler   CGI Executor
                             |               |
                  Filesystem (www/...)   CGI scripts (cgi-bin)
                             \               /
                              v             v
                              HttpResponse
                                   |
                                   v
                           Client::_outBuffer
                                   |
                        EPOLLOUT + send()
                                   |
                                   v
                        Browser receives response
```

---

## 9. Daru – Config & Routing (parse, match, apply rules)

Esta sección es específica para la parte de **configuración** (Daru). Son 3 minidiagramas:

- 9.1 Pipeline de parsing del `.conf`
- 9.2 Árbol de routing (cómo se elige server + location)
- 9.3 Cadena de reglas del `RequestProcessor` aplicando la config

### 9.1 ConfigParser Pipeline (texto → estructuras C++)

#### Nodes

1. `webserv.conf` (archivo texto)  
2. `ConfigParser`  
3. `Preprocessed content` (sin comentarios, normalizado)  
4. `Raw server blocks` (`server { ... }` como texto)  
5. `ServerConfig[]`  
6. `LocationConfig[]` dentro de cada `ServerConfig`  

#### Edges

- `webserv.conf` → `ConfigParser`  
  - Label: **read file**

- `ConfigParser` → `Preprocessed content`  
  - Label: **strip comments / normalize whitespace / validate braces**

- `Preprocessed content` → `Raw server blocks`  
  - Label: **split by "server { ... }"**

- `Raw server block` → `ServerConfig`  
  - Label: **parse listen, server_name, root, index, error_page, max_body_size...**

- `ServerConfig` → `LocationConfig[]`  
  - Label: **parse each "location" block**

#### ASCII sketch

```text
webserv.conf (text)
    |
    v
 ConfigParser
    |
    v
Preprocessed content (no comments, braces checked)
    |
    v
Raw "server { ... }" blocks
    |
    v
ServerConfig[]
    |
    v
LocationConfig[]   (per server)
```

---

### 9.2 Routing Tree – host/path → Server + Location

#### Nodes

1. `HttpRequest (Host, Port, Path)`  
2. `ServerConfig[]`  
3. `Matched ServerConfig`  
4. `LocationConfig[]`  
5. `Matched LocationConfig`  
6. `Attributes` de la location:
   - `root`
   - `allowed_methods`
   - `max_body_size`
   - `autoindex`
   - `upload_store`
   - `cgi_handlers`
   - `redirect (return code/url)`

#### Edges

- `HttpRequest` → `ServerConfig[]`  
  - Label: **select by listenPort + Host header**

- `ServerConfig[]` → `Matched ServerConfig`  
  - Label: **first server that matches host/port**

- `Matched ServerConfig` → `LocationConfig[]`  
  - Label: **candidate locations (path prefixes)**

- `LocationConfig[]` → `Matched LocationConfig`  
  - Label: **longest prefix match on request path**

- `Matched LocationConfig` → cada atributo (`root`, `allowed_methods`, etc.)  
  - Label: **rules that will drive the handler**

#### ASCII sketch

```text
HttpRequest (Host, Port, Path)
        |
        v
  ServerConfig[]
        |
        v   (by host:port)
Matched ServerConfig
        |
        v   (by longest path prefix)
Matched LocationConfig  (/ , /cgi-bin , /uploads ...)
        |
        +--> root
        +--> allowed_methods
        +--> max_body_size
        +--> autoindex
        +--> upload_store
        +--> cgi_handlers
        +--> redirect (code/url)
```

---

### 9.3 RequestProcessor Rule Chain (cómo se aplican las reglas)

Este diagrama enseña la “cadena de decisiones” que aplica `RequestProcessor` usando la config elegida.

#### Nodes

1. `HttpRequest`  
2. `RequestProcessor::process()`  
3. `Select ServerConfig`  
4. `Select LocationConfig`  
5. `Check allowed_methods`  
6. `Check max_body_size`  
7. `Check redirect (return)`  
8. `Check CGI handler`  
9. `Static / Autoindex / Upload handler`  
10. `CGI Executor`  
11. `HttpResponse`  

#### Edges (en orden)

- `HttpRequest` → `RequestProcessor::process()`  
  - Label: **entry point**

- `RequestProcessor` → `Select ServerConfig`  
  - Label: **host + port**

- `Select ServerConfig` → `Select LocationConfig`  
  - Label: **longest prefix match**

- `Select LocationConfig` → `Check allowed_methods`  
  - Label: **if method not allowed → 405**

- `Check allowed_methods` → `Check max_body_size`  
  - Label: **if method OK**

- `Check max_body_size` → `HttpResponse (413)`  
  - Label: **if body too large**

- `Check max_body_size` → `Check redirect`  
  - Label: **if body size OK**

- `Check redirect` → `HttpResponse (3xx)`  
  - Label: **if location has return code/url**

- `Check redirect` → `Check CGI handler`  
  - Label: **if no redirect**

- `Check CGI handler` → `CGI Executor`  
  - Label: **if extension mapped to cgi_handlers**

- `Check CGI handler` → `Static / Autoindex / Upload handler`  
  - Label: **else (no CGI)**

- `Static / Autoindex / Upload handler` → `HttpResponse`  
  - Label: **file/dir/upload response**

- `CGI Executor` → `HttpResponse`  
  - Label: **wrap CGI output**

#### ASCII sketch – cadena de reglas

```text
HttpRequest
    |
    v
RequestProcessor::process()
    |
    v
Select ServerConfig (by host:port)
    |
    v
Select LocationConfig (longest prefix)
    |
    v
Check allowed_methods
    | allowed?         \
    v                  \
Check max_body_size     --> HttpResponse 405
    | ok?              \
    v                   \
Check redirect (return)  \
    | has redirect?       \
    v                      \
HttpResponse 3xx            \
                             v
                        Check CGI handler
                        /             \
                       /               \
               CGI Executor    Static/Autoindex/Upload
                       \               /
                        v             v
                          HttpResponse
```

Idea para la defensa: puedes contar que Daru “no sirve nada por sí sola”, sino que:

- **Parsea** el mapa de red (ServerConfig + LocationConfig).
- **Encuentra** el server y la location correctos para cada request.
- **Aplica una cadena de reglas** (método permitido, tamaño, redirect, CGI o estático) que condiciona qué hace luego el código de Ana (HTTP/Client) y Carles (CGI/epoll). 

---

## 8. Request / Response Queue in Client (handleRead vs handleWrite)

Este diagrama muestra cómo `Client` gestiona **varias respuestas por conexión** usando:

- Un buffer de salida inmediato: `Client::_outBuffer`
- Una cola de respuestas futuras: `Client::_responseQueue` (`PendingResponse`)

### Nodes

1. `epoll (EPOLLIN)`  
2. `Client::handleRead()`  
3. `HttpParser`  
4. `HttpRequest`  
5. `Client::handleCompleteRequest()`  
6. `RequestProcessor`  
7. `HttpResponse`  
8. `Client::enqueueResponse()`  
9. `Client::_outBuffer`  
10. `Client::_responseQueue`  
11. `epoll (EPOLLOUT)`  
12. `Client::handleWrite()`  
13. `send(fd, _outBuffer)`  
14. `Browser / HTTP Client`  

### Flujo básico

1. **Lectura y parseo**
   - `epoll (EPOLLIN)` → `Client::handleRead()`  
     - Label: **recv(fd)**  
   - `Client::handleRead()` → `HttpParser`  
     - Label: **_parser.consume(bytes)**  
   - `HttpParser` → `HttpRequest`  
     - Label: **state == COMPLETE**

2. **Construcción de respuesta(s)**
   - `HttpRequest` → `Client::handleCompleteRequest()`  
     - Label: **request parsed**
   - `Client::handleCompleteRequest()` → `RequestProcessor`  
     - Label: **process(request, configs)**  
   - `RequestProcessor` → `HttpResponse`  
     - Label: **static / autoindex / error OR CGI-handled**

3. **Encolado en `_outBuffer` o `_responseQueue`**
   - `HttpResponse` → `Client::enqueueResponse()`  
     - Label: **serialize() → vector<char>**

   Dentro de `enqueueResponse`:

   - Si `_outBuffer` está vacío:
     - `HttpResponse` → `Client::_outBuffer`  
       - Label: **first response becomes active**  
       - Side effect: **`_state = STATE_WRITING_RESPONSE`**

   - Si `_outBuffer` NO está vacío:
     - `HttpResponse` → `Client::_responseQueue`  
       - Label: **push PendingResponse(data, closeAfter)**

   Casos típicos donde se usan **varias respuestas en cola**:
   - `Expect: 100-continue` → primero se encola la respuesta `100 Continue`, luego la respuesta final con 200/4xx.
   - **HTTP pipelining** o varias peticiones en la misma conexión → varias `HttpResponse` se encolan en orden.

4. **Envío gradual cuando el socket está listo**

   - `epoll (EPOLLOUT)` → `Client::handleWrite()`  
     - Label: **socket writable**

   - `Client::handleWrite()` → `send(fd, _outBuffer)`  
     - Label: **write as much as possible**

   - Si `send()` no envía todo:
     - `_outBuffer` queda con la parte pendiente  
     - **No se toca la cola**, la siguiente `PendingResponse` aún no sube.

   - Si `_outBuffer` queda vacío:
     - Si `_closeAfterWrite == true` → `STATE_CLOSED`
     - Si `_responseQueue` NO está vacía:
       - `Client::_responseQueue` → `Client::_outBuffer`  
         - Label: **pop next PendingResponse**  
       - Se mantiene `STATE_WRITING_RESPONSE` hasta vaciarla.
     - Si no hay más respuestas → `STATE_IDLE`

### ASCII sketch – por qué usar una cola

```text
EPOLLIN
  |
  v
Client::handleRead()
  |
  v
HttpParser  -->  HttpRequest  -->  handleCompleteRequest()
                                       |
                                       v
                                RequestProcessor
                                       |
                                       v
                                  HttpResponse
                                       |
                                       v
                             enqueueResponse(data, closeAfter)
                             /                      \
                 _outBuffer empty?                no
                     |                            |
                     v                            v
             _outBuffer = data           _responseQueue.push(data)
             state = WRITING

EPOLLOUT
  |
  v
Client::handleWrite()
  |
  v
send(fd, _outBuffer)  --> erase sent bytes
  |
  v
if _outBuffer empty:
    if closeAfterWrite: CLOSE
    else if queue not empty:
        pop next into _outBuffer (keep WRITING)
    else:
        state = IDLE
```

### Motivo de diseño (qué puedes explicar al evaluador)

- **No bloqueamos en send()**: como usas `epoll` y sockets no bloqueantes, `send()` puede enviar solo parte de la respuesta. Mantener todo en `_outBuffer` + cola evita mezclar lógicas de envío en `handleRead()`.
- **Orden garantizado**: si llegan varias peticiones en la misma conexión (pipeline) o necesitas mandar primero un `100 Continue` y luego la respuesta real, la cola garantiza que las respuestas salen en el orden correcto.
- **Separación de responsabilidades**:
  - `handleRead()` → solo **lee**, parsea y encola respuestas.
  - `handleWrite()` → solo **envía** lo que haya en `_outBuffer` y va sacando la cola cuando corresponda.
- **Flexibilidad para CGI**: cuando una request lanza un CGI, puedes encolar otras respuestas (por ejemplo de peticiones previas) y seguir consumiendo datos sin bloquear la conexión mientras esperas al script. La cola centraliza todo el envío saliente. 

