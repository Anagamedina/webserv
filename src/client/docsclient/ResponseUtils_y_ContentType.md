## `ResponseUtils::fillBaseResponse` y `Content-Type`

Este archivo explica la función:

```cpp
void fillBaseResponse(HttpResponse& response, const HttpRequest& request,
                      int statusCode, bool shouldClose,
                      const std::vector<char>& body);
```

y, en concreto, **qué hace con el `Content-Type`**.

---

### 1. Qué hace `fillBaseResponse`

Código actual:

```cpp
void fillBaseResponse(HttpResponse& response, const HttpRequest& request, int statusCode,
                      bool shouldClose, const std::vector< char >& body) {
    response.setStatusCode(statusCode);
    response.setVersion(versionToString(request.getVersion()));
    if (shouldClose)
        response.setHeader("Connection", "close");
    else
        response.setHeader("Connection", "keep-alive");
    // Si el caller ya ha fijado un Content-Type concreto (por ejemplo,
    // StaticPathHandler usando la extension real del fichero o un CGI
    // que ha enviado sus propios headers), no lo sobreescribimos.
    if (!response.hasHeader("content-type"))
        response.setContentType(request.getPath());
    response.setBody(body);
}
```

Pasos:

1. **`setStatusCode`**  
   - Pone el código HTTP (200, 404, 500, etc.).
2. **`setVersion`**  
   - Pone `HTTP/1.0` o `HTTP/1.1` según la request.
3. **`Connection`**  
   - Si `shouldClose` es `true` → `Connection: close`.  
   - Si no → `Connection: keep-alive`.
4. **`Content-Type` (aquí estaba la duda)**  
   - Solo llama a `setContentType(request.getPath())` si **NO** existe ya un header `Content-Type`.
5. **`setBody`**  
   - Copia el cuerpo (vector de bytes) a la respuesta.

---

### 2. ¿Por qué comprobar `hasHeader("content-type")`?

Hay dos tipos de respuestas en tu servidor:

1. **Estáticas** (archivos de disco: `.html`, `.css`, `.png`, etc.).
2. **Dinámicas**:
   - CGI: el script devuelve sus propios headers.
   - Autoindex: tú generas un HTML a mano y pones `Content-Type: text/html`.

Si `fillBaseResponse` **siempre** hiciera:

```cpp
response.setContentType(request.getPath());
```

entonces:

- Podrías **pisar**:
  - El `Content-Type` que ha puesto un CGI.
  - El `Content-Type` que ha puesto `StaticPathHandler` para un index.
  - El `Content-Type` que se haya fijado en una respuesta de error especial.

Por eso se añadió:

```cpp
if (!response.hasHeader("content-type"))
    response.setContentType(request.getPath());
```

Traducción:

- “Si NADIE ha puesto todavía un `Content-Type`, lo calculo yo usando la ruta de la request.”
- “Si YA hay un `Content-Type`, lo respeto y no lo cambio.”

---

### 3. Quién pone el `Content-Type` antes

Ejemplos:

#### 3.1. Archivos estáticos

En `StaticPathHandler.cpp`:

- Para un index:

```cpp
if (!readFileToBody(indexPath, body)) { ... }
response.setContentType(indexPath); // Usa la extension real del fichero.
```

- Para un archivo normal:

```cpp
if (!readFileToBody(path, body)) { ... }
response.setContentType(path); // Igual: se basa en la extension.
```

En estos casos, **el `Content-Type` ya está puesto** antes de llamar a `fillBaseResponse`, así que `hasHeader("content-type")` devuelve `true` y **no se toca**.

#### 3.2. CGI

En `ClientCgi.cpp`:

- Se parsean los headers devueltos por el CGI:

```cpp
parseCgiHeaders(_cgiProcess->getResponseHeaders(), _response);
_response.setBody(_cgiProcess->getResponseBody());
```

Si el script CGI devuelve un header `Content-Type`, entonces `parseCgiHeaders` lo mete en `_response` y de nuevo **no queremos pisarlo**.

Como en el flujo CGI no se llama a `fillBaseResponse`, no hay problema, pero la idea es la misma:  
**si ya hay Content-Type, se respeta.**

---

### 4. ¿Qué pasa cuando NADIE pone `Content-Type`?

En casos sencillos (por ejemplo, una respuesta de error muy básica que solo llama a `buildErrorResponse` sin fijar tipo), `fillBaseResponse` se encarga de calcularlo:

```cpp
if (!response.hasHeader("content-type"))
    response.setContentType(request.getPath());
```

`setContentType` mira la **extensión** del “filename” que le pasas:

- `.html` → `text/html`
- `.css` → `text/css`
- `.js` → `application/javascript`
- etc.

Si no reconoce la extensión → `application/octet-stream`.

---

### 5. Resumen para recordar

- `fillBaseResponse` es una **función genérica** para rellenar respuestas.
- Solo pone `Content-Type` cuando **nadie más lo ha puesto**.
- La lógica de “qué tipo es este archivo” vive en `HttpResponse::setContentType`.
- Para respuestas estáticas:
  - `StaticPathHandler` llama a `response.setContentType(ruta_del_archivo)`.
- Para CGI:
  - Se usan los headers que devuelve el propio script CGI.

Así evitas sobrescribir `Content-Type` y al mismo tiempo tienes un fallback razonable cuando no se ha definido explícitamente.

