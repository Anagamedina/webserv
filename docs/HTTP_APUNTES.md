## Apuntes de HTTP y Cookies (para webserv)

> Documento para ir rellenando poco a poco mientras avanzas.  
> Idea: usarlo como guía de estudio + checklist de implementación.

---

## 1. Visión general de mi parte en el proyecto

- **¿Cuál es mi responsabilidad?**
  - [ ] Entender dónde encaja la parte HTTP dentro del servidor
  - [ ] Saber qué entra (bytes desde socket) y qué sale (respuestas HTTP)
  - [ ] Diferenciar: **red** vs **HTTP** vs **lógica de aplicación**

- **Resumen rápido del flujo (para que lo dibuje):**
  - Socket/epoll → `ServerManager` (lee bytes)
  - `HttpParser` → convierte bytes en `HttpRequest`
  - Lógica (routing, ficheros, CGI…) → genera `HttpResponse`
  - Se serializa `HttpResponse` → bytes → socket

Espacio para mi propio esquema:

- [ ] Dibujar el diagrama de cajas: `socket → parser → request → response → socket`

---

## 2. Estructura de una petición HTTP (request)

- **Start-line (línea de inicio)**:
  - Forma general: `METHOD SP REQUEST-TARGET SP HTTP-VERSION CRLF`
  - Ejemplo: `GET /hello?name=42 HTTP/1.1`

- **Headers (cabeceras)**:
  - Forma general: `Nombre: valor`
  - Terminan con una línea vacía (`\r\n`)
  - Ejemplos importantes:
    - `Host`
    - `Content-Length`
 a   - `Content-Type`
    - `Cookie`

- **Body (cuerpo)**:
  - Solo existe en algunos métodos (POST, PUT…)
  - Normalmente controlado por `Content-Length`

Checklist de comprensión (marcar cuando lo entienda bien):

- [ ] Puedo reconocer una start-line correcta y una incorrecta  
- [ ] Sé qué significa `HTTP/1.1` y por qué es importante  
- [ ] Sé qué significa `Content-Length`  
- [ ] Sé cómo se separan headers y body (doble `\r\n`)  

---

## 3. Estructura de una respuesta HTTP (response)

- **Status line (línea de estado)**:
  - Forma general: `HTTP-VERSION SP STATUS-CODE SP REASON-PHRASE CRLF`
  - Ejemplo: `HTTP/1.1 200 OK`

- **Headers**:
  - Ejemplos:
    - `Content-Length`
    - `Content-Type`
    - `Set-Cookie`
    - `Connection`

- **Body**:
  - Contenido que ve el cliente (HTML, JSON, texto, etc.)

Checklist:

- [ ] Sé qué es un código de estado (200, 404, 500…)  
- [ ] Puedo escribir una respuesta HTTP mínima a mano  
- [ ] Entiendo la diferencia entre headers y body  

---

## 4. Mis clases principales (diseño conceptual)

> Aquí voy a describir cómo quiero que sean mis clases, **sin código**, solo idea.

### 4.1. `HttpRequest`

- **Qué debería guardar**:
  - [ ] Método (`GET`, `POST`, …)
  - [ ] Ruta / path (`/`, `/login`, `/img/logo.png`)
  - [ ] Query string (`?a=1&b=2`) si la necesito
  - [ ] Versión (`HTTP/1.1`)
  - [ ] Headers (mapa nombre → valor)
  - [ ] Body (string o buffer)
  - [ ] Cookies (mapa nombre → valor) — ver sección de cookies

- Preguntas para pensar:
  - ¿Quiero un enum para el método?
  - ¿Cómo quiero acceder a un header? (`getHeader("Host")`?)
  - ¿Qué hago si falta un header obligatorio?

Espacio para notas propias sobre `HttpRequest`:

---

### 4.2. `HttpResponse`

- **Qué debería guardar**:
  - [ ] Código de estado (200, 404, 500…)
  - [ ] Texto de estado (`OK`, `Not Found`, …)
  - [ ] Versión (`HTTP/1.1`)
  - [ ] Headers
  - [ ] Body
  - [ ] Cookies para enviar (`Set-Cookie`)

- Preguntas para pensar:
  - ¿Cómo voy a asegurar que `Content-Length` coincide con el tamaño del body?
  - ¿Voy a tener helpers para respuestas típicas? (200 OK, 404 Not Found…)

Espacio para notas propias sobre `HttpResponse`:

---

### 4.3. `HttpParser`

- Responsabilidades:
  - [ ] Recibir trozos de texto (bytes) desde el socket
  - [ ] Acumularlos hasta tener una petición completa
  - [ ] Parsear:
    - [ ] Línea de inicio
    - [ ] Headers
    - [ ] Body (cuando lo implemente)
  - [ ] Indicar si:
    - La petición está **completa**
    - Falta datos (esperar más bytes)
    - Es inválida (error 400)

- Idea de estados internos (state machine):
  - [ ] `PARSE_START_LINE`
  - [ ] `PARSE_HEADERS`
  - [ ] `PARSE_BODY`
  - [ ] `DONE`

Espacio para dibujar la máquina de estados:

---

## 5. Cookies (parte importante mía)

### 5.1. Leer cookies (desde la request)

- Cabecera de entrada:
  - Ejemplo:  
    `Cookie: sessionId=abc123; theme=dark; lang=es`

- Pasos para parsear (idea):
  - [ ] Leer valor completo de `Cookie`
  - [ ] Separar por `;`
  - [ ] Para cada parte, separar `nombre=valor`
  - [ ] Hacer trim de espacios
  - [ ] Guardar en algo tipo `cookies["sessionId"] = "abc123"`

- Checklist:
  - [ ] Sé localizar la cabecera `Cookie` en los headers
  - [ ] Sé cómo pasar de la string de cookie a un mapa nombre→valor

Notas y ejemplos propios:

---

### 5.2. Escribir cookies (en la response)

- Cabecera de salida:
  - Ejemplo simple:  
    `Set-Cookie: sessionId=abc123`
  - Ejemplo con atributos:  
    `Set-Cookie: sessionId=abc123; Path=/; HttpOnly; Max-Age=3600`

- Atributos típicos:
  - [ ] `Path`
  - [ ] `Domain`
  - [ ] `Max-Age` / `Expires`
  - [ ] `Secure`
  - [ ] `HttpOnly`
  - [ ] `SameSite`

- Diseño conceptual:
  - [ ] Una estructura conceptual para una cookie (nombre, valor, atributos)
  - [ ] Una forma de añadir cookies a la respuesta (lista de cookies → varias cabeceras `Set-Cookie`)

Notas propias sobre cómo quiero modelar cookies:

---

## 6. Integración con el servidor actual

> Aquí apunto cómo mi parte va a hablar con el resto del servidor.

- **Entrada**:
  - Bytes leídos en `ServerManager::handleClientData(fd)`
  - Esos bytes se pasan a mi parser HTTP

- **Salida**:
  - `HttpRequest` ya construido → se pasa a la capa de lógica (aún por definir)
  - `HttpResponse` generado por la lógica (cuando exista) → lo serializo a texto y se envía al socket

Checklist:

- [ ] Entiendo desde dónde se llamará a mi `HttpParser`  
- [ ] Entiendo quién va a usar `HttpRequest` y `HttpResponse`  

Notas sobre integración:

---

## 7. Plan de trabajo (para mí)

1. **Teoría básica** (solo leer y anotar)
   - [ ] Apuntar ejemplos de peticiones/respuestas reales (con `curl -v`)
   - [ ] Rellenar secciones 2 y 3 de este documento con ejemplos

2. **Diseño de estructuras (sin código todavía)**
   - [ ] Rellenar secciones 4.1 y 4.2 con el diseño de `HttpRequest` y `HttpResponse`
   - [ ] Rellenar 5.1 y 5.2 con el flujo de cookies

3. **Diseño del parser**
   - [ ] Dibujar la máquina de estados de `HttpParser`
   - [ ] Decidir cómo voy a tratar datos parciales (cuando no llega todo de una vez)

4. **Cuando ya me sienta cómoda con el diseño → empezar a codificar**
   - Pero siempre con este `.md` como guía.

---

## 8. Dudas que quiero revisar con el tiempo

- [ ] ¿Cómo manejar errores de parsing in a forma limpia? (400 Bad Request)  
- [ ] ¿Qué mínimo de métodos HTTP necesito soportar para el subject?  
- [ ] ¿Qué requisitos exactos hay sobre cookies en el subject?  
- [ ] ¿Necesito soportar `Transfer-Encoding: chunked`?  

Espacio para nuevas dudas:


