# CGI (flujo simple)

## Roles
- **CgiExecutor**: crea el proceso CGI y los pipes.
- **CgiProcess**: guarda fds y buffers (body y respuesta).
- **Client**: registra fds en epoll y lee/escribe.
- **ServerManager**: solo gestiona epoll y reenvia eventos al Client.

---

## Paso a paso (lo minimo que hago yo)
1) **Client** llama `CgiExecutor::executeAsync(...)`
2) Obtengo un `CgiProcess` con `pipe_in` y `pipe_out`
3) Registro en epoll:
   - `pipe_in` con `EPOLLOUT` (para escribir el body)
   - `pipe_out` con `EPOLLIN` (para leer la salida)
4) Cuando epoll avisa:
   - Si es `pipe_in`: escribo el body en partes
   - Si es `pipe_out`: leo la salida en partes
5) Cuando termina:
   - Construyo el `HttpResponse`
   - Envio la respuesta al cliente

---

## Resumen ultra corto
**Client crea CGI → registra fds → lee/escribe → arma HttpResponse**.
