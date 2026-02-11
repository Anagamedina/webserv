# Apuntes de Cookies (webserv)

> Guía de estudio y referencia para la implementación de cookies y sesiones en el proyecto.

---

## 1. ¿Qué son las cookies?

Las **cookies** son pequeños fragmentos de datos que el **servidor** envía al cliente y que el **navegador** (o cliente HTTP) guarda y reenvía automáticamente en peticiones posteriores al mismo dominio.

**Uso típico:**
- Identificar sesiones de usuario
- Mantener el "estado" entre peticiones (HTTP es sin estado)
- Preferencias, carritos de compra, inicio de sesión, etc.

---

## 2. Headers clave

| Header      | Dirección        | Significado                                              |
|------------|------------------|----------------------------------------------------------|
| **Set-Cookie** | Servidor → Cliente | El servidor pide al cliente que guarde esta cookie     |
| **Cookie**     | Cliente → Servidor | El cliente reenvía las cookies guardadas en la petición |

### Formato Set-Cookie (respuesta)

```
Set-Cookie: nombre=valor; Path=/; Option1=valor1
```

- **nombre=valor** — nombre de la cookie y su valor
- **Path=/** — ruta para la que se envía la cookie (ej. `Path=/` = todo el sitio)
- Otras opciones: `Max-Age`, `Secure`, `HttpOnly`, `SameSite`

### Formato Cookie (petición)

```
Cookie: nombre1=valor1; nombre2=valor2
```

- Lista de `nombre=valor` separados por `;`
- El cliente concatena todas las cookies que aplican al dominio/ruta

---

## 3. Flujo básico en nuestro proyecto

```
┌─────────┐                    ┌─────────┐
│ Cliente │                    │ Servidor│
└────┬────┘                    └────┬────┘
     │  1. GET /index.html          │
     │     (sin Cookie)             │
     │─────────────────────────────>│
     │                              │  ¿Tiene cookie "id" válida? NO
     │                              │  → Generar nuevo id
     │                              │  → Guardar en memoria
     │  2. 200 OK                   │
     │     Set-Cookie: id=abc123; Path=/
     │<─────────────────────────────│
     │                              │
     │  (cliente guarda la cookie)   │
     │                              │
     │  3. GET /index.html           │
     │     Cookie: id=abc123         │
     │─────────────────────────────>│
     │                              │  ¿Tiene cookie "id" válida? SÍ
     │                              │  → No añadir Set-Cookie
     │  4. 200 OK                   │
     │     (sin Set-Cookie)          │
     │<─────────────────────────────│
```

---

## 4. Implementación en el código

### Archivos implicados

| Archivo           | Responsabilidad                                      |
|-------------------|------------------------------------------------------|
| `SessionUtils.hpp` | Declaración de `addSessionCookieIfNeeded()`          |
| `SessionUtils.cpp` | Generación de ID, parseo de Cookie, almacén, Set-Cookie |
| `ResponseUtils.cpp`| Llama a `addSessionCookieIfNeeded()` desde `fillBaseResponse()` |
| `HttpRequest`      | Guarda headers, incluyendo `Cookie`                 |
| `HttpResponse`     | Añade header `Set-Cookie`                            |

### Flujo en el código

1. **Request llega** → `HttpParser` lee el header `Cookie` y lo guarda en `HttpRequest`.
2. **Se construye la respuesta** → `RequestProcessor` → `fillBaseResponse()`.
3. **fillBaseResponse()** llama a `addSessionCookieIfNeeded(response, request, statusCode)`.
4. **addSessionCookieIfNeeded()**:
   - Solo actúa si `statusCode` es 2xx (200–299).
   - Lee `request.getHeader("cookie")`.
   - Busca `id=XXX` en la cadena.
   - Si no hay cookie o el `id` no está en el almacén → genera uno nuevo, lo guarda, añade `Set-Cookie`.
   - Si ya tiene sesión válida → no hace nada.

### Funciones principales

```cpp
// Genera un ID único: timestamp_counter_random
std::string createSessionId();

// Extrae "valor" de "Cookie: id=valor" o "id=valor; otros=..."
std::string extractIdFromCookie(const std::string& cookieHeader);

// Añade Set-Cookie si el cliente no tiene sesión válida
void addSessionCookieIfNeeded(HttpResponse& response, const HttpRequest& request, int statusCode);
```

### Almacén de sesiones

- `static std::set<std::string> validSessions`
- Se guardan los `id` que hemos generado y enviado.
- Si el cliente vuelve con ese ID en `Cookie`, lo reconocemos como sesión válida.
- **Nota:** En esta implementación las sesiones no expiran (en memoria durante la ejecución del servidor).

---

## 5. Ejemplo de headers reales

### Primera petición (sin cookie)

**Request:**
```
GET /index.html HTTP/1.1
Host: localhost

```

**Response:**
```
HTTP/1.1 200 OK
Content-Type: text/html
Set-Cookie: id=1739123456_0_789012; Path=/
Content-Length: 1234

<!DOCTYPE html>...
```

### Segunda petición (con cookie)

**Request:**
```
GET /index.html HTTP/1.1
Host: localhost
Cookie: id=1739123456_0_789012

```

**Response:**
```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1234

<!DOCTYPE html>...
```

(No se envía `Set-Cookie` porque la sesión ya es válida.)

---

## 6. Cómo probar

Ejecutar el servidor y el script de verificación:

```bash
# Terminal 1: arrancar servidor
./webserver config/default.conf

# Terminal 2: probar cookies
python3 tests/scripts/verify.py
```

El test de cookies en `verify.py`:
1. Hace GET sin cookie → espera `Set-Cookie` con `id=...`
2. Hace GET con esa cookie → espera 200 OK

---

## 7. Checklist de comprensión

- [ ] Entiendo la diferencia entre `Cookie` (request) y `Set-Cookie` (response)
- [ ] Sé qué es una sesión y por qué usamos cookies para mantenerla
- [ ] Conozco el flujo: primera visita → Set-Cookie → visita posterior → Cookie
- [ ] Sé en qué archivos está implementada la lógica de sesiones
- [ ] Entiendo cuándo se añade `Set-Cookie` (solo en respuestas 2xx y si no hay sesión válida)
