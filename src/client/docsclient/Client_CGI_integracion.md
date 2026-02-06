# Integracion CGI en Client (que hace y por que)

## Idea
El `Client` es el que **inicia** el CGI porque es quien tiene el socket del cliente
y el acceso al `ServerManager` para registrar fds en `epoll`.

---

## Flujo implementado
1) `Client::buildResponse()` decide si es CGI.
2) Si es CGI:
   - Llama a `CgiExecutor::executeAsync`
   - Guarda el `CgiProcess`
   - Registra `pipe_out` (EPOLLIN) y `pipe_in` (EPOLLOUT) en `ServerManager`
3) `ServerManager` recibe eventos y llama a `Client::handleCgiPipe()`
4) `Client`:
   - Escribe el body al CGI cuando hay EPOLLOUT
   - Lee la salida cuando hay EPOLLIN
   - Cuando termina, construye el `HttpResponse` y lo deja listo para enviar

---

## Puntos clave
- El `RequestProcessor` solo decide si es CGI o no.
- El `Client` maneja los fds porque tiene el socket.
- El `ServerManager` solo registra y despacha eventos de `epoll`.

---

## TODO (pendiente)
- Obtener `interpreterPath` desde la config (cuando exista).
- Mejorar parseo de headers CGI si hace falta.
