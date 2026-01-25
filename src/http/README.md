# Guía rápida: Parseo de solicitudes HTTP (HTTP/1.1)

Explicación pensada para empezar desde cero y entender qué hace el parser 

## 1) ¿Qué es una HTTP request?
- Es un mensaje de texto plano que viaja por TCP hasta el servidor.
- El servidor recibe bytes crudos y debe interpretarlos (método, ruta, headers, body).
- Ejemplo (lo que verías con `curl -v http://example.com/`):
```
GET /index.html HTTP/1.1
Host: example.com
User-Agent: curl/7.68.0
Accept: */*

```

## 2) Estructura fija (3 partes)
- Request line: `METHOD SP URI SP VERSION` (ej. `GET /photos/cat.jpg?size=big HTTP/1.1`).
- Headers: líneas `Clave: Valor` hasta una línea vacía (`\r\n\r\n` marca el fin de headers).
  - Ejemplos: `Host` (obligatorio en HTTP/1.1), `Content-Length`, `Content-Type`, `Cookie`.
- Body: opcional. Solo si el método lo usa (POST/PUT). Tamaño indicado por `Content-Length`.

## 3) ¿Por qué parsear?
- Los datos llegan en trozos; no siempre recibes todo de golpe.
- Necesitas convertir el texto crudo en información usable:
  - Método (GET/POST/DELETE), ruta y query.
  - Headers normalizados.
  - Cookies separadas en pares nombre=valor.
  - Body con la longitud correcta.
  - Errores detectados a tiempo (400/405/414/505, etc.).

## 4) Flujo de parsing (visión rápida)
```
[bytes de red] -> [buffer]
  └─ acumula hasta ver "\r\n\r\n" (fin de headers)
       └─ parsea request line -> método | URI | versión
            └─ parsea headers (clave: valor)
                 └─ detecta cookies (split por ";" y "=")
                 └─ si Content-Length -> lee body exacto
                       └─ valida límites y formato
                             └─ request lista o error
```

### Pasos detallados
1) Acumula datos: junta en un buffer hasta encontrar `\r\n\r\n` (fin de headers).  
2) Request line: divide por espacios → método, URI, versión.  
   - URI → separa path (`/algo`) de query (`?a=1`).  
3) Headers: procesa cada línea hasta la vacía.  
   - Header names son case-insensitive; guarda con un formato consistente.  
4) Cookies: si hay header `Cookie`, separa por `;` y cada par por `=`.  
5) Body: si existe `Content-Length: N`, lee exactamente N bytes luego de los headers.  
6) Validación: método permitido, URI válida, versión soportada, tamaños bajo los límites.  
7) Estado final: request completa lista para routing/response o se responde error.

## 5) Conceptos clave antes de codear
- Separadores: todo usa CRLF (`\r\n`); headers terminan con `\r\n\r\n`.
- Sensibilidad: método y versión son case-sensitive; headers no lo son.
- Non-blocking: el parser debe manejar datos parciales (Epoll + lectura incremental).
- Máquina de estados: recuerda en qué etapa estás (leyendo línea inicial, headers, body, completo, error).
- Límites: define tamaños máximos de línea, headers y body para evitar abusos.

## 6) Resultado que busca el parser
Transformar algo como:
```
GET /about.html HTTP/1.1\r\nHost: mysite.com\r\nCookie: user=ana\r\n\r\n
```
En un objeto con campos útiles:
- method = GET
- path = /about.html
- query = (vacío)
- host = mysite.com
- cookies = { user: ana }
- body = (vacío)

## 7) Lo que implementé (mi parte)
- Implementé un parser incremental con máquina de estados en `HttpParser`:
  - Entrada principal en `consume()`.
  - Start line en `HttpParserStartLine.cpp`.
  - Headers en `HttpParserHeaders.cpp`.
  - Body (Content-Length y chunked) en `HttpParserBody.cpp`.
- Separé `path` y `query` desde la start line para dejarlo listo para CGI y routing.
- Normalicé headers a minúsculas y validé `Host` obligatorio en HTTP/1.1.
- Preparé tests manuales para validar el parser y el request:
  - `tests/manual_http_request.cpp`
  - `tests/manual_http_parser.cpp`
- Añadí targets en CMake para correr tests en CLion:
  - `test_http_request`
  - `test_http_parser`

## 8) Apuntes / documentación que fui escribiendo
- `docsHttp/start-line.md`: request line, URI, query string.
- `docsHttp/headers.md`: headers, Content-Length, chunked y errores.
- `docsHttp/body.md`: body, Content-Type y tipos comunes.
- `docsHttp/body-parser-flow.md`: flujo del parser del body según funciones actuales.

