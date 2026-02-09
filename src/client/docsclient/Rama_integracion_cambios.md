# Cambios que hice en la rama de integración (network + cgi + main)

## Objetivo
Quería que esta rama funcionara con mi `Client` y mi `HttpRequest`
sin tocar nada en `client/` ni en `http/`.

---

## 1) ServerManager (network)

### Archivo: `src/network/ServerManager.hpp`
- **Cambio:** el constructor ahora recibe `const std::vector<ServerConfig>* configs`.
- **Cambio:** guardo `configs_` como puntero a la lista de servers.
- **Eliminado:** el uso de `ServerBlock` y `port_configs_`.

**Por qué:** Daru usa `ServerConfig` directo (no `ServerBlock`) y mi `Client`
espera `std::vector<ServerConfig>`.

### Archivo: `src/network/ServerManager.cpp`
- **Cambio:** el constructor recorre `configs_` para levantar listeners por puerto.
- **Cambio:** `handleNewConnection()` crea `Client(fd, configs_, port)`.
- **Cambio:** quité `setServerManager()` porque mi `Client` no lo tiene.
- **Cambio:** comenté `handleCgiPipe()` (placeholder) para no romper.
- **Cambio:** `Client::CLOSED` → `STATE_CLOSED` (enum real).

**Por qué:** mantener compatibilidad con mi `Client` sin tocarlo.

---

## 2) CGI (CgiExecutor)

### Archivo: `src/cgi/CgiExecutor.cpp`
- **Cambio:** eliminé `StringUtils.hpp` (no existe en repo).
- **Cambio:** `request.getMethod()` (enum) → string con helper `methodToString`.
- **Cambio:** uso `request.getPath() + request.getQuery()` para construir URI.
- **Cambio:** `request.getBody()` (`vector<char>`) → `string` con `bodyToString`.
- **Cambio:** `CONTENT_LENGTH` lo genero con `ostringstream`.
- **Cambio:** añadí `<cctype>` para `toupper()`.

**Por qué:** adaptar el CGI a mi `HttpRequest` real sin tocar HTTP.

---

## 3) main.cpp (arranque mínimo)

### Archivo: `src/main.cpp`
- **Cambio:** creo un `std::vector<ServerConfig>` temporal con un server en 8080.
- **Cambio:** `ServerManager` se instancia con esa lista.
- **Cambio:** eliminé `server.start()` (ya no existe en la versión nueva).
- **Añadido:** includes de `<vector>` y `"config/ServerConfig.hpp"`.

**Por qué:** poder ejecutar el server sin ConfigParser y sin tocar Client/HTTP.

---

## 4) Cambios NO realizados (a propósito)
- **No** toqué `Client`.
- **No** toqué `HttpRequest` ni `HttpResponse`.
- **No** implementé CGI en `RequestProcessor` (solo adaptación base).

---

## Resultado actual
La rama integra `network/` y `cgi/` con mi API de `client/http` sin romper firmas.
Me sirve como base para el merge con Daru.

---

## Próximos pasos opcionales
- Añadir los `.cpp` de `cgi/` al `Makefile` cuando queramos compilar CGI.
- Si se decide CGI async real: añadir `handleCgiPipe` en `Client`.
