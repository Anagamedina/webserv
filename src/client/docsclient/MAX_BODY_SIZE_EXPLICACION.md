# Explicación: max_body_size (client_max_body_size)

> Cómo Daru extrae el límite de la config y cómo se usa en el parser HTTP.
> Incluye por qué lo gestionamos en `parseBodyChunked`.

---

## 1. ¿Qué es max_body_size?

La directiva `client_max_body_size` (o `max_body_size` en la config) define el **tamaño máximo** en bytes que puede tener el body de una petición HTTP (POST, PUT, etc.).

Ejemplo en la config:

```nginx
server {
    listen 8080;
    client_max_body_size 1M;   /* 1 megabyte = 1048576 bytes */
    # ...
}
```

Si el cliente envía un body mayor, el servidor debe rechazar con **413 Request Entity Too Large**.

---

## 2. Cómo lo extrae Daru (config)

### 2.1 Directiva en el archivo .conf

```
client_max_body_size 1M;
client_max_body_size 1048576;
client_max_body_size 10k;
```

### 2.2 Flujo de parseo

| Paso | Archivo | Función | Qué hace |
|------|---------|---------|----------|
| 1 | `ConfigParser.cpp` | `parseSingleServerBlock()` | Detecta la directiva `client_max_body_size` |
| 2 | `ConfigParser.cpp` | `parseMaxSizeBody(server, tokens)` | Recibe el valor (ej. `"1M"`) y lo pasa a `ConfigUtils::parseSize()` |
| 3 | `ConfigUtils.cpp` | `parseSize(str)` | Convierte `"1M"`, `"10k"`, `"1048576"` en bytes |
| 4 | `ServerConfig.cpp` | `setMaxBodySize(size)` | Guarda el valor en `max_body_size_` |
| 5 | `ServerConfig` | `getMaxBodySize()` | Devuelve el valor cuando se necesita |

### 2.3 parseSize() – formato admitido

```cpp
// ConfigUtils::parseSize()
"1048576"  → 1048576 bytes
"1k"       → 1024 bytes
"1M"       → 1048576 bytes
"10k"      → 10240 bytes
"2g"       → 2147483648 (hasta INT_MAX)
```

Sufijos: `k` (×1024), `M` (×1024²), `g` (×1024³). Sin sufijo = bytes.

### 2.4 Dónde se guarda

```cpp
// ServerConfig.hpp
size_t max_body_size_;   // en bytes

// ServerConfig.cpp - constructor
max_body_size_(config::section::max_body_size)  // valor por defecto
```

---

## 3. Cómo lo uso yo (parser HTTP)

### 3.1 Conexión config → parser

El `Client` obtiene el server con `selectServerByPort(port, configs)` y pasa el límite al parser en el **constructor** (una sola vez, al crearse la conexión):

```cpp
// Client.cpp - constructor
Client::Client(int fd, ..., configs, listenPort) : ... {
  const ServerConfig* server = selectServerByPort(listenPort, configs);
  if (server) _parser.setMaxBodySize(server->getMaxBodySize());
}
```

El límite no cambia durante la vida de la conexión, así que basta con fijarlo al conectar.

### 3.2 Validación con Content-Length

Si el body viene con `Content-Length` (POST clásico):

```cpp
if (_maxBodySize > 0 && _contentLength > _maxBodySize)
    return false;   // → _state = ERROR, respondemos 413
```

El rechazo ocurre antes de leer un solo byte del body.

### 3.3 Validación con Transfer-Encoding: chunked

Con chunked no existe `Content-Length`. El body se recibe en trozos hasta un chunk `0\r\n`. No podemos saber el tamaño total hasta que termina.

Por eso la comprobación no puede estar solo en `validateHeaders()`. Hay que hacerla al ir leyendo chunks.

---

## 4. Por qué lo gestionamos en parseBodyChunked

### 4.1 Diferencia entre Content-Length y chunked

| Tipo | Cuándo sabemos el tamaño | Dónde validar |
|------|---------------------------|---------------|
| Content-Length | En los headers | `validateHeaders()` |
| Chunked | Al final del último chunk | Durante el parseo del body |

### 4.2 Formato chunked

```
4\r\n
Wiki\r\n
5\r\n
pedia\r\n
0\r\n
\r\n
```

Cada chunk es `tamaño_hex\r\ndatos\r\n`. El último es `0\r\n` (y luego `\r\n`). No hay un tamaño total previo.

### 4.3 Dónde comprobar el límite

En `handleChunkDataState()` se añaden los datos al body:

```cpp
// HttpParserBody.cpp - handleChunkDataState()
if (_chunkSize > 0)
    _request.addBody(_buffer.begin(), _buffer.begin() + _chunkSize);
// ...

// Comprobar después de CADA chunk añadido
if (_maxBodySize > 0 && _request.getBody().size() > _maxBodySize) {
    _state = ERROR;
    return false;
}
```

### 4.4 Ventajas de comprobar en cada chunk

1. Se rechaza en cuanto se supera el límite, sin esperar al final.
2. Se evita acumular en memoria un body enorme.
3. El cliente recibe 413 cuanto antes.
4. El límite se aplica tanto a body fijo como a chunked.

### 4.5 Doble capa en parseBodyFixedLength

También comprobamos en `parseBodyFixedLength()` **antes** de leer:

```cpp
if (_maxBodySize > 0 && _contentLength > _maxBodySize) {
  _state = ERROR;
  return;
}
```

Defensa en profundidad: aunque `validateHeaders()` ya rechaza, esta capa extra protege si hay algún flujo donde se llegue al body sin pasar por esa validación.

---

## 5. Flujo completo (resumen)

```
[Config .conf]
   client_max_body_size 1M;
        ↓
[ConfigParser]  parseMaxSizeBody() → parseSize("1M")
        ↓
[ServerConfig]  max_body_size_ = 1048576
        ↓
[Client]  selectServerByPort(port, configs) → setMaxBodySize(server->getMaxBodySize())
        ↓
[HttpParser]  _maxBodySize = 1048576
        ↓
┌─────────────────────────────────────────────────────────────┐
│ Content-Length: 2000000  →  validateHeaders: 2000000 > 1M    │
│                              → ERROR, no leemos body         │
├─────────────────────────────────────────────────────────────┤
│ Transfer-Encoding: chunked  →  validateHeaders: OK           │
│   Chunk 500k  →  body = 500k   → OK                         │
│   Chunk 600k  →  body = 1.1M   → handleChunkDataState:      │
│                                  body > 1M → ERROR           │
└─────────────────────────────────────────────────────────────┘
```

---

## 6. Archivos implicados

| Responsable | Archivo | Función / responsabilidad |
|-------------|---------|----------------------------|
| Daru | `ConfigParser.cpp` | `parseMaxSizeBody()` – detecta directiva |
| Daru | `ConfigUtils.cpp` | `parseSize()` – convierte "1M" a bytes |
| Daru | `ServerConfig.cpp` | `setMaxBodySize()`, `getMaxBodySize()` |
| Yo | `HttpParser.hpp/cpp` | `_maxBodySize`, `setMaxBodySize()` |
| Yo | `Client.cpp` | `selectServerByPort()` + `setMaxBodySize()` antes de consume |
| Yo | `HttpParserHeaders.cpp` | `validateHeaders()` – valida Content-Length |
| Yo | `HttpParserBody.cpp` | `parseBodyFixedLength()` + `handleChunkDataState()` – valida body |

---

## 7. Visión "Laser Attack" para la defensa ante el evaluador

**Pregunta:** "¿Cómo evitas que te manden un archivo infinito?"

**Respuesta en dos capas:**

- **Capa estática (Content-Length):** "En cuanto recibo el header Content-Length, lo comparo con `_maxBodySize`. Si es mayor, disparo el error antes de guardar ni un byte del body. Lo compruebo en `validateHeaders()` y también en `parseBodyFixedLength()` como defensa en profundidad."

- **Capa dinámica (Chunked):** "Con chunked no conozco el tamaño total de antemano. Voy acumulando trozos en `handleChunkDataState()`. En cuanto el acumulado supera el límite, detengo la recepción y marco ERROR."

---

## 8. Nota C++98 (parseChunkSizeLine)

Se usa `std::strtoul` para el tamaño del chunk en hexadecimal. Con C++98, tener en cuenta que el valor no supere lo que admite `std::size_t` en la arquitectura (para un `max_body_size` de unos pocos MB no hay problema).
