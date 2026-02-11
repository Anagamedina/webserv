# Scripts de test Python del proyecto

Resumen de los scripts de prueba que añadió el equipo, qué prueban y qué puede faltar en el proyecto.

---

## 1. Ubicación de los scripts

| Script             | Ubicación                                   |
|--------------------|---------------------------------------------|
| `stress_test.py`   | Raíz + `tests/scripts/` (duplicado)         |
| `verify.py`        | Raíz + `tests/scripts/` (duplicado)         |
| `debug_cgi.py`     | Raíz + `tests/scripts/` (duplicado)         |
| `test_cgi_pipe.py` | Raíz + `tests/scripts/` (duplicado)         |
| `test_concurrent.py` | Solo en `tests/scripts/`                  |

**Recomendación:** Usar los de `tests/scripts/` y eliminar los duplicados de la raíz.

---

## 2. Qué hace cada script

### stress_test.py

**Qué prueba:** Carga del servidor con peticiones concurrentes.

- 50 hilos, 20 peticiones por hilo = **1000 peticiones totales**
- Conexión a `127.0.0.1:8080`
- Envía `GET /index.html HTTP/1.1` con `Host: localhost`, `Connection: close`
- Comprueba que la respuesta contenga `200 OK`
- Mide tiempo total y requests/segundo
- **Exit 0** si todas pasan, **exit 1** si alguna falla

**Cómo ejecutar:**
```bash
# Primero arrancar el servidor en puerto 8080
python3 stress_test.py
# o desde tests/scripts:
python3 tests/scripts/stress_test.py
```

**Qué verifica en el proyecto:**
- El servidor resiste concurrencia sin crashear
- Múltiples conexiones simultáneas
- Respuestas 200 OK ante requests válidos

---

### verify.py

**Qué prueba:** Transferencia chunked y cookies/sesiones.

| Test                    | Qué hace                                                              |
|-------------------------|-----------------------------------------------------------------------|
| **Chunked Encoding**    | POST con `Transfer-Encoding: chunked` al endpoint indicado             |
| **Cookies & Sessions**   | Comprueba `Set-Cookie` en primera request y reutilización en la 2ª     |

**Detalle Chunked:**
- POST a `/cgi-bin/echo.py` con body en chunks (`4\r\nWiki\r\n`, `5\r\npedia\r\n`, etc.)
- **Nota:** El proyecto tiene `echo_body.py`, no `echo.py`. Hay que ajustar el path o el nombre del script.

**Detalle Cookies:**
- Pide GET `/index.html` sin cookie
- Busca header `Set-Cookie` con `session_id=...`
- Repite GET con la cookie
- Comprueba 200 OK (sesión reutilizada)

**Cómo ejecutar:**
```bash
python3 verify.py
```

**Qué verifica en el proyecto:**
- Soporte de `Transfer-Encoding: chunked` en requests POST
- Parser de body chunked
- Uso de `Set-Cookie` para sesiones
- Uso de la cookie `session_id` en requests posteriores

---

### test_concurrent.py

**Qué prueba:** Comportamiento con muchos clientes y desconexiones.

| Test                      | Qué hace                                                   |
|---------------------------|------------------------------------------------------------|
| **Test 1**                | 20 requests concurrentes GET a `/?req=N`                   |
| **Test 2**                | 10 requests; la mitad desconecta antes de leer respuesta   |
| **Test 3**                | 10 clientes lentos (delay 0.5s entre connect y request)    |
| **Test 4**                | 50 ciclos rápidos connect/close (sin request)              |

**Cómo ejecutar:**
```bash
python3 tests/scripts/test_concurrent.py
```

**Qué verifica en el proyecto:**
- Manejo de varias conexiones simultáneas
- Clientes que cierran conexión de golpe
- Clientes lentos sin provocar timeouts/crashes
- Conexiones que se abren y cierran sin enviar HTTP completo

---

### debug_cgi.py

**Qué prueba:** Ejecución manual de scripts CGI para depurar el flujo.

- Ejecuta directamente `www/cgi-bin/test.py`
- Prueba con variables de entorno (REQUEST_METHOD, QUERY_STRING, etc.)
- Simula pipes stdin/stdout como en CGI

**Advertencia:** Contiene ruta hardcodeada `/home/carles/Documents/42bcn/webserv`. Habría que cambiarla o hacerla configurable.

**Uso:** Para depuración interna de CGI, no para validar el webserver en producción.

---

### test_cgi_pipe.py

**Qué prueba:** Simula el flujo de `CgiExecutor` con `fork`, `pipe`, `dup2`, `execve`.

- Crea pipes
- Hace fork del proceso
- El hijo redirige stdin/stdout, configura entorno y ejecuta el script CGI
- El padre lee la salida por el pipe

**Advertencia:** Misma ruta hardcodeada `/home/carles/...`.

**Uso:** Para entender y depurar el pipeline CGI, no como test automático del webserver.

---

## 3. Qué puede faltar en el proyecto

Según lo que prueban estos scripts:

| Funcionalidad             | Script que lo prueba   | Estado probable          |
|---------------------------|------------------------|--------------------------|
| Concurrencia estable      | stress_test, test_concurrent | Revisar epoll/threads |
| Peticiones GET básicas    | stress_test            | Debería estar            |
| Transfer-Encoding chunked | verify                 | Puede faltar parsing     |
| Cookies / sesiones        | verify                 | Puede faltar implementar |
| CGI POST con body chunked | verify                 | Requiere CGI + chunked   |
| Clientes que desconectan  | test_concurrent        | Revisar fd cerrados      |
| Manejo de conexiones lentas | test_concurrent      | Revisar timeouts         |
| Ruta `/cgi-bin/echo.py`   | verify                 | Existe `echo_body.py`    |

---

## 4. Dónde está (o faltaría) cada funcionalidad en el código

### Peticiones GET básicas — **Implementado**

| Dónde | Archivo | Qué hace |
|-------|---------|----------|
| Lectura del request | `src/client/Client.cpp` | `handleRead()` → `_parser.consume()` |
| Parseo start line | `src/http/HttpParserStartLine.cpp` | Detecta método GET |
| Procesamiento | `src/client/RequestProcessor.cpp` | Matching → servir estático o CGI |
| Respuesta estática | `src/client/StaticPathHandler.cpp` | Lee archivo, construye respuesta |

**Conclusión:** El flujo GET básico está completo. El stress_test debería pasar si el servidor responde 200 a `/index.html`.

---

### Transfer-Encoding chunked — **Implementado**

| Dónde | Archivo | Qué hace |
|-------|---------|----------|
| Detección del header | `src/http/HttpParserHeaders.cpp` | `handleHeader()` línea 42: si `Transfer-Encoding` contiene `chunked` → `_isChunked = true` |
| Parseo del body | `src/http/HttpParserBody.cpp` | `parseBodyChunked()`, `handleChunkSizeState()`, `handleChunkDataState()`, `handleChunkEndState()` |
| Body acumulado | `HttpRequest::addBody()` | El body decodificado se guarda en `_request` |

**Conclusión:** El parsing de chunked está implementado. Si `verify.py` (test 1) falla, comprobar:
1. **Path del CGI:** usa `/cgi-bin/echo.py` pero el proyecto tiene `echo_body.py` → cambiar a `/cgi-bin/echo_body.py` en `verify.py`, o crear un alias/symlink.
2. **Config:** que la location `/cgi-bin/` esté configurada como CGI y apunte al directorio correcto.

---

### Cookies / sesiones — **Implementado**

| Dónde | Archivo | Qué hace |
|-------|---------|----------|
| Generación de session_id | `SessionUtils.cpp` | `generateSessionId()` con timestamp + contador + random |
| Extracción de cookie | `SessionUtils.cpp` | `extractSessionIdSimple()` parsea `Cookie: session_id=valor` |
| Almacén de sesiones válidas | `SessionUtils.cpp` | `static std::set<std::string> validSessions` |
| Añadir Set-Cookie | `SessionUtils.cpp` | `addSessionCookieIfNeeded()` — solo en respuestas 2xx |
| Integración | `ResponseUtils.cpp` | `fillBaseResponse()` llama a `addSessionCookieIfNeeded()` |

---

### CGI POST con body chunked — **Implementado**

El flujo combina dos piezas:

| Fase | Archivo | Qué hace |
|------|---------|----------|
| 1. Parseo chunked | `src/http/HttpParserBody.cpp` | Convierte el body chunked en body completo en `request.getBody()` |
| 2. Pasar body al CGI | `src/cgi/CgiExecutor.cpp` | Línea 154: `bodyToString(request.getBody())` → se pasa al `CgiProcess` |
| 3. Escribir al stdin del hijo | `src/cgi/CgiProcess.cpp` | El padre escribe el body al pipe que es stdin del proceso CGI |
| 4. Variables de entorno | `CgiExecutor::prepareEnvironment()` | `CONTENT_LENGTH` = `request.getBody().size()`, `CONTENT_TYPE` si existe |

El script CGI (`echo_body.py`) lee `CONTENT_LENGTH` y `sys.stdin.read(length)` — el body ya llega decodificado (sin formato chunked).

**Si falla el test de verify.py (chunked):**
- Comprobar path: `/cgi-bin/echo.py` vs `/cgi-bin/echo_body.py`.
- Comprobar que la config tenga una location CGI para `/cgi-bin/` que ejecute scripts `.py`.

---

## 5. Checklist de funcionalidades

| Funcionalidad                      | ¿Implementada? | Dónde en el código                          |
|------------------------------------|----------------|---------------------------------------------|
| Múltiples conexiones simultáneas   | Sí (epoll)     | `src/network/ServerManager.cpp`, `EpollWrapper` |
| Peticiones GET básicas             | Sí             | `Client` → `HttpParser` → `RequestProcessor` → `StaticPathHandler` |
| Transfer-Encoding: chunked        | Sí             | `HttpParserHeaders.cpp` + `HttpParserBody.cpp` |
| Set-Cookie / session_id            | Sí (implementado) | `SessionUtils.cpp` + `ResponseUtils.cpp` |
| CGI con body chunked               | Sí             | Parser decodifica → `CgiExecutor` pasa `request.getBody()` al CGI |
| Path verify.py (echo.py)           | Ajustar        | Cambiar a `echo_body.py` o crear `echo.py`   |

---

## 6. Cómo ejecutar todos

```bash
# 1. Arrancar el servidor (en otra terminal)
./webserver config/default.conf   # o tu config con puerto 8080

# 2. Stress test
python3 tests/scripts/stress_test.py

# 3. Chunked + cookies
python3 tests/scripts/verify.py

# 4. Concurrencia y desconexiones
python3 tests/scripts/test_concurrent.py
```

---

## 7. Ajustes recomendados

1. **verify.py:** Usar `/cgi-bin/echo_body.py` o crear `echo.py` si el test espera ese path.
2. **debug_cgi.py y test_cgi_pipe.py:** Sustituir rutas hardcodeadas por rutas relativas al proyecto.
3. **Duplicados:** Borrar de la raíz los scripts que ya están en `tests/scripts/`.
