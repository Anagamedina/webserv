# Apuntes de repaso: Epoll y CGI

> Resumen para entender la capa de red (epoll) y la ejecución de scripts CGI.

---

## 1. Epoll: ¿qué es y para qué sirve?

**epoll** es la API de Linux para I/O multiplexado: observar muchos file descriptors (sockets) y saber **cuáles están listos** para leer/escribir sin bloquear el proceso.

- **Problema:** Un servidor puede tener cientos de clientes; no podemos bloquear esperando a uno.
- **Solución:** `epoll_wait()` despierta cuando hay actividad en alguno de los fd registrados.

### Conceptos clave

| Concepto | Significado |
|----------|-------------|
| **epoll_fd** | El descriptor que representa la instancia epoll (`epoll_create`) |
| **epoll_ctl** | Añadir/modificar/eliminar un fd de la observación |
| **epoll_wait** | Bloquear hasta que haya eventos en los fd registrados |
| **Level Triggered (LT)** | Notifica mientras el fd siga listo (modo por defecto) |
| **Edge Triggered (ET)** | Notifica solo al cambiar de estado |

En el proyecto usamos **Level Triggered** para mayor simplicidad y seguridad.

---

## 2. Archivos de la capa Epoll

| Archivo | Responsabilidad |
|---------|-----------------|
| `EpollWrapper.cpp/hpp` | Abstracción sobre epoll: `addFd()`, `modFd()`, `removeFd()`, `wait()` |
| `ServerManager.cpp/hpp` | Bucle principal, reparte eventos entre listeners, clients y pipes CGI |
| `TcpListener.cpp/hpp` | Socket de escucha: `bind()`, `listen()`, `accept()` |

---

## 3. Flujo del bucle principal (ServerManager::run)

```
while (true) {
    num_events = epoll_.wait(events, MAX_EVENTS, 3000);

    for (cada evento) {
        fd = evento.data.fd;

        if (fd es listener)     → handleNewConnection(fd);
        else if (fd es client)   → handleClientEvent(fd, eventos);
        else if (fd es cgi_pipe) → handleCgiPipeEvent(fd, eventos);
    }

    checkTimeouts();  // Cierra clientes inactivos
}
```

### Tipos de fd y eventos

| Tipo | Eventos típicos | Acción |
|------|-----------------|--------|
| **Listener** | `EPOLLIN` | `accept()`, crear `Client`, añadir a epoll con `EPOLLIN | EPOLLRDHUP` |
| **Client** | `EPOLLIN`, `EPOLLOUT`, `EPOLLERR`, `EPOLLHUP`, `EPOLLRDHUP` | Leer (`handleRead`) o escribir (`handleWrite`) |
| **CGI pipe** | `EPOLLIN` (leer stdout), `EPOLLOUT` (escribir stdin) | `handleCgiPipe()` |

---

## 4. EpollWrapper: operaciones básicas

```cpp
// Crear instancia (main/epoll_fd)
epoll_create(1);

// Registrar fd para eventos
addFd(fd, EPOLLIN | EPOLLRDHUP);

// Cambiar eventos (ej. activar EPOLLOUT cuando hay datos para escribir)
modFd(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);

// Desregistrar (al cerrar conexión)
removeFd(fd);

// Esperar eventos (timeout en ms)
int n = wait(events, MAX_EVENTS, 3000);
```

Regla importante con LT: solo pedir `EPOLLOUT` cuando el cliente tenga datos pendientes (`needsWrite()`), para evitar eventos continuos.

---

## 5. CGI: qué es y cómo encaja

**CGI (Common Gateway Interface)** permite ejecutar programas externos (scripts) para generar respuestas dinámicas. El servidor:
1. Recibe la petición HTTP
2. Ejecuta el script con variables de entorno (REQUEST_METHOD, QUERY_STRING, etc.)
3. Pasa el body por stdin
4. Lee la salida (headers + body) por stdout
5. Convierte eso en respuesta HTTP

### Diseño no bloqueante

- **fork()** inmediato: no bloqueamos esperando al script
- **Pipes** en modo non-blocking
- Los pipes se registran en epoll: cuando hay datos en stdout del hijo, epoll avisa
- El cliente sigue atendido por el mismo bucle de eventos

---

## 6. Archivos CGI

| Archivo | Responsabilidad |
|---------|-----------------|
| `CgiExecutor.cpp/hpp` | `executeAsync()`: crea pipes, hace fork, prepara env, execve en el hijo |
| `CgiProcess.cpp/hpp` | Rastrea el proceso hijo: escribe body al stdin, lee stdout, parsea headers, detecta timeout |

---

## 7. Flujo CGI (pasos)

```
1. Client detecta petición CGI (startCgiIfNeeded)
   └─> CgiExecutor::executeAsync(request, scriptPath, interpreterPath)

2. CgiExecutor:
   ├─> pipe(pipe_in), pipe(pipe_out)
   ├─> Pipes en non-blocking
   ├─> fork()
   │   HIJO: dup2(stdin/stdout), chdir, setenv, alarm(5), execve(script)
   │   PADRE: close(ends no usados), crea CgiProcess(body, pipe_in[1], pipe_out[0])
   └─> Devuelve CgiProcess*

3. Client registra pipes en ServerManager:
   └─> registerCgiPipe(pipe_out, EPOLLIN)   // leer salida
   └─> registerCgiPipe(pipe_in, EPOLLOUT)  // escribir body

4. Epoll avisa cuando hay eventos:
   └─> handleCgiPipe(): lee stdout O escribe body a stdin
   └─> Cuando headers completos + body leído → finalizeCgiResponse()
   └─> unregisterCgiPipe(), enqueueResponse(), limpieza
```

---

## 8. Variables de entorno CGI (prepareEnvironment)

| Variable | Origen |
|----------|--------|
| `GATEWAY_INTERFACE` | "CGI/1.1" |
| `REQUEST_METHOD` | GET, POST, DELETE |
| `SCRIPT_FILENAME` | Ruta absoluta del script |
| `SCRIPT_NAME` | Parte path de la URI |
| `QUERY_STRING` | Todo después de `?` |
| `CONTENT_LENGTH` | Tamaño del body |
| `CONTENT_TYPE` | Header Content-Type |
| `REQUEST_URI` | URI completa |
| `HTTP_*` | Resto de headers (Host → HTTP_HOST) |

---

## 9. CgiProcess: estados y funciones

- **appendResponseData()**: Acumula bytes del stdout del hijo
- **tryParseHeaders()**: Busca `\r\n\r\n`, extrae headers y body, parsea `Status: XXX`
- **isTimedOut()**: Comprueba si el hijo lleva más de 5 s ejecutando
- El padre escribe el body al pipe_in en fragmentos (epoll EPOLLOUT cuando hay espacio)

---

## 10. Checklist de repaso

- [ ] Sé qué es epoll y por qué usamos Level Triggered
- [ ] Conozco el bucle principal de ServerManager y los 3 tipos de fd
- [ ] Entiendo addFd/modFd/removeFd y cuándo se usa cada uno
- [ ] Sé qué es CGI y el flujo fork → pipes → execve
- [ ] Conozco las variables de entorno que recibe el script CGI
- [ ] Entiendo por qué los pipes CGI se registran en epoll (I/O no bloqueante)
