## CGI lado cliente – plan de trabajo

### 1. Objetivo de tu parte

- El **Client** debe:
  - Detectar cuándo una petición es **CGI** (según config y extensión).
  - Lanzar el proceso CGI usando `CgiExecutor`.
  - Conectarse a los **pipes** del CGI (escritura del body y lectura de la respuesta).
  - Construir un `HttpResponse` con la salida del CGI.
  - Enviar la respuesta al cliente TCP, igual que una respuesta estática.

El **RequestProcessor** solo decide “esto es CGI” o “esto es estático”;  
el **Client** es quien hace el trabajo de I/O y coordinación con `ServerManager`.

---

### 2. Archivos que tocan tu parte

- `src/client/Client.hpp` / `Client.cpp`
  - Flujo general de lectura/escritura del socket.
  - Lógica para montar la respuesta (`buildResponse`, `handleCompleteRequest`).
  - Integración con CGI (`startCgiIfNeeded`, `handleCgiPipe`, `finalizeCgiResponse`).

- `src/client/ClientCgi.cpp`
  - Implementación concreta de `startCgiIfNeeded`, `handleCgiPipe`, `finalizeCgiResponse`.

- `src/client/RequestProcessorUtils.hpp/.cpp`
  - Helpers: `selectServerByPort`, `matchLocation`, `resolvePath`.
  - Detección de CGI: `isCgiRequest`, `isCgiRequestByConfig`, `getFileExtension`.

- `src/client/RequestProcessor.cpp`
  - Usa `isCgiRequest(...)` / `isCgiRequestByConfig(...)` para saber si la ruta real es CGI.
  - Hoy para CGI devuelve 501 (porque el flujo real lo hace el `Client`).

- `src/cgi/CgiExecutor.hpp/.cpp` y `src/cgi/CgiProcess.hpp/.cpp`
  - Parte de Carles: `fork/exec`, creación de pipes, almacenamiento de headers/body CGI.
  - Tú solo llamas a su interfaz.

---

### 3. Flujo CGI paso a paso (vista cliente)

#### 3.1. Recepción y parsing de la petición

1. `ServerManager` recibe evento `EPOLLIN` en el socket del cliente.
2. Llama a `Client::handleRead()`.
3. `Client::handleRead()`:
   - Lee bytes del socket a `_inBuffer`.
   - Los pasa al `HttpParser` (`_parser.consume(...)`).
   - Cuando el parser marca `COMPLETE`, llama a `handleCompleteRequest()`.

#### 3.2. Decisión CGI o estático

4. `Client::handleCompleteRequest()`:
   - Obtiene el `HttpRequest` desde `_parser`.
   - Llama a `buildResponse()`.

5. `Client::buildResponse()`:
   - Llama primero a `startCgiIfNeeded(request)`.
   - Si devuelve `true`, es porque:
     - Se ha arrancado un CGI correctamente **o**
     - Ha fallado el CGI y ya se ha construido un error.
   - Si devuelve `false`, entonces:
     - Llama a `_processor.process(...)` → flujo estático normal.

#### 3.3. startCgiIfNeeded – conexión con config

6. `Client::startCgiIfNeeded(const HttpRequest& request)` (en `ClientCgi.cpp`):
   - Selecciona `ServerConfig` con `selectServerByPort(_listenPort, _configs)`.
   - Hace `matchLocation(*server, request.getPath())`.
   - Resuelve la ruta real del script:
     - `std::string scriptPath = resolvePath(*server, location, request.getPath());`
   - Decide si es CGI:
     - `isCgiRequest(scriptPath)` → reglas simples (.py/.php).
     - `isCgiRequestByConfig(location, scriptPath)` → usa el mapa `cgi_handler` de la location.
   - Si no es CGI: devuelve `false` y el Client sigue con estático.
   - Si es CGI:
     - Calcula `interpreterPath` con `location->getCgiPath(ext)` usando `getFileExtension(scriptPath)`.
     - Llama a `CgiExecutor exec; _cgiProcess = exec.executeAsync(request, scriptPath, interpreterPath);`
     - Si `_cgiProcess == 0`: construye error 500 con `buildErrorResponse(...)` y devuelve `true`.
     - Si todo bien:
       - Pide a `ServerManager` registrar:
         - `pipeOut` con `EPOLLIN | EPOLLRDHUP`.
         - `pipeIn` con `EPOLLOUT | EPOLLRDHUP`.
       - Cambia `_state = STATE_READING_BODY`.
       - Devuelve `true` para indicar que la respuesta se completará asincrónicamente.

#### 3.4. Manejo de los pipes CGI

7. `ServerManager` detecta eventos en los pipes y llama a `Client::handleCgiPipe(int pipe_fd, size_t events)`:

   - Si es `pipeIn` y hay `EPOLLOUT`:
     - Coge el body de la request desde `_cgiProcess->getRequestBody()`.
     - Escribe en el pipe en trozos (`write(...)`).
     - Actualiza `_cgiProcess->advanceBodyBytesWritten(...)`.
     - Cuando ha escrito todo:
       - Cierra `pipeIn` (`_cgiProcess->closePipeIn()`).
       - `ServerManager->unregisterCgiPipe(pipe_fd)`.

   - Si es `pipeOut` y hay `EPOLLIN`:
     - Lee con `read(pipe_fd, buffer, sizeof(buffer))`.
     - Si hay datos:
       - Llama a `_cgiProcess->appendResponseData(...)`.
     - Si `bytes == 0` (EOF):
       - Cierra `pipeOut` y lo desregistra.
       - Llama a `finalizeCgiResponse()`.
       - Libera `_cgiProcess`.

#### 3.5. Construcción de la HttpResponse final

8. `Client::finalizeCgiResponse()`:
   - Obtiene el `HttpRequest` para decidir keep-alive.
   - Setea:
     - `statusCode` desde `_cgiProcess->getStatusCode()`.
     - `version` según `request.getVersion()`.
     - Header `Connection` (`close` o `keep-alive`).
   - Parsea los headers de la salida CGI (`parseCgiHeaders(...)`) y los mete en `HttpResponse`.
   - Mete el body CGI con `_response.setBody(_cgiProcess->getResponseBody());`
   - Serializa y encola:
     - `std::vector<char> serialized = _response.serialize();`
     - `enqueueResponse(serialized, shouldClose);`

9. A partir de ahí, el flujo de envío de respuesta es el mismo que uno estático:
   - `Client::handleWrite()` va sacando `_outBuffer` por `send(...)` hasta que se envía todo.

---

### 4. Qué falta o hay que revisar para cumplir el subject (parte cliente/CGI)

Basado en el subject típico de `webserv` y en tus docs de `docsclient`:

1. **Cobertura de métodos y cuerpos para CGI**
   - Asegurar que:
     - GET sin body funciona (solo env vars / query string).
     - POST con body se envía completo al CGI (ya hay soporte parcial con pipes).
   - Verificar que el tamaño del body respeta `client_max_body_size` antes de lanzar CGI.

2. **Variables de entorno CGI (lado CgiExecutor/CgiProcess)** //CARLES
    - Aunque lo implemente Carles, tú debes entender que:
     - `CgiExecutor` debe rellenar `envp` con:
       - `REQUEST_METHOD`, `SCRIPT_FILENAME`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`,
       - `SERVER_PROTOCOL`, `SERVER_NAME`, `SERVER_PORT`, etc.
   - Como client, asegúrate de que el `HttpRequest` tiene toda la info necesaria (headers, path, query, body).

3. **Integración completa con config (`cgi_handler`)**
   - Ya se usa `isCgiRequestByConfig(...)` y `location->getCgiPath(ext)`.
   - Revisa que:
     - La config de ejemplo (`/cgi-bin`) tiene `cgi_handler` correcto.
     - Se soportan al menos `.py` y cualquier extensión usada en el subject.

4. **Manejo de errores de CGI**
   - Casos a probar:
     - El script hace `exit(1)` o lanza excepción → debe devolver 500 (o el status apropiado).
     - El script no escribe headers válidos → fallback a 500.
   - Verificar que `buildErrorResponse(...)` se usa correctamente cuando `_cgiProcess` es `0` o cuando la salida es inválida.

5. **Timeouts de CGI**
   - El subject de `webserv` exige que el servidor no se quede bloqueado para siempre.
   - Desde el lado `Client`:
     - `_lastActivity` ya existe; se puede usar para matar CGI si tarda demasiado (junto con `ServerManager`).
   - Pendiente: definir política de timeout razonable y cómo se traduce en 504/500.

6. **Coexistencia CGI / estático**
   - Ya tienes:
     - Detección de index CGI en directorios y redirección para que pase por el flujo CGI.
   - Revisa:
     - Rutas mixtas (`/directory/index.py` vs `/directory/otro.html`).
     - Que no se sirve un CGI como si fuera texto plano.

7. **Tests funcionales**
   - Crear una pequeña batería de pruebas manuales:
     - `GET /cgi-bin/hello.py`
     - `GET /cgi-bin/test.py?foo=bar`
     - `POST /cgi-bin/test.py` con body sencillo.
     - Script que imprime muchas cabeceras y body grande.
   - Verificar:
     - Status line.
     - Headers CGI (Content-Type, etc.).
     - Cuerpo completo.

---

### 5. Checklist rápido para ti (lado cliente)

1. [ ] Entender y documentar (ya casi hecho) `startCgiIfNeeded`, `handleCgiPipe`, `finalizeCgiResponse`.
2. [ ] Probar CGI simple (`hello.py`) con GET.
3. [ ] Probar CGI que lee body con POST.
4. [ ] Verificar que las rutas CGI se detectan por:
   - [ ] Extensión simple (`.py`, `.php`).
   - [ ] Mapa `cgi_handler` en `LocationConfig`.
5. [ ] Probar comportamiento cuando el script falla (500).
6. [ ] Anotar posibles mejoras:
   - Timeouts.
   - Logs de debug más claros.
   - Uso potencial de `hasPendingData()` en el loop principal.

Con este plan tienes una guía clara de tu parte de CGI en el cliente y qué puntos revisar para estar alineada con el subject de `webserv`.

