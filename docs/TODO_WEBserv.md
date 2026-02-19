## TODO global – proyecto webserv

Lista viva para ir marcando lo que falta o se va completando.

---

### 1. HTTP / core del servidor

- [ ] **Timeouts de cliente y CGI**
  - [ ] Definir política de timeout (segundos) para conexiones inactivas.
  - [ ] Usar `_lastActivity` en `Client` + bucle de `ServerManager` para cerrar clientes colgados.
  - [ ] Conectar con `CgiProcess::isTimedOut()` para cortar CGI demasiado lentos.

- [ ] **Mejorar tabla de `Content-Type`**
  - [ ] Añadir más extensiones comunes en `HttpResponse::setContentType` (pdf, json, ico, etc.).

- [ ] **Cabecera Date**
  - [ ] Añadir header `Date:` en `HttpResponse::serialize()` según el subject de `webserv`.

---

### 2. Error handling / páginas de error

- [x] **Usar `error_page` de config** para servir HTML de error real (`ErrorUtils.cpp`).
- [x] **Fallback HTML** cuando el archivo de error no existe.
- [ ] **Status adicionales**
  - [x] 413 `Request Entity Too Large` (límite de body).
  - [ ] Revisar que todos los códigos usados (301/302/403/404/405/413/500) tengan reason phrase.

---

### 3. CGI – parte cliente / processor

- [x] **Detección de CGI**
  - [x] Por extensión simple (`isCgiRequest`).
  - [x] Por mapa de config (`isCgiRequestByConfig` + `cgi_handler`).

- [x] **Envío de body a CGI**
  - [x] GET sin body → funciona (body vacío).
  - [x] POST con body → se envía completo mediante `CgiProcess` + `handleCgiPipe`.

- [x] **Límite `client_max_body_size` antes de CGI**
  - [x] Validar en `startCgiIfNeeded` y devolver 413 si se supera.

- [ ] **Validación de intérprete**
  - [ ] Si `location->getCgiPath(ext)` devuelve vacío, decidir:
    - [ ] Responder 500/501 con mensaje claro.
    - [ ] O no tratar la ruta como CGI.

- [ ] **Pruebas funcionales CGI**
  - [ ] `GET /cgi-bin/hello.py`.
  - [ ] `GET /cgi-bin/test.py?foo=42`.
  - [ ] `POST /cgi-bin/test.py` con body pequeño.
  - [ ] `POST` con body mayor que `client_max_body_size` → 413.

---

### 4. CGI – parte Carles (solo para seguimiento)

- [ ] **Manejo de errores en `CgiExecutor` / `CgiProcess`**
  - [ ] Script que hace `exit(1)` → marcar error y provocar 500.
  - [ ] Script sin headers válidos → fallback a 500.

- [ ] **Variables de entorno completas**
  - [ ] Confirmar que `prepareEnvironment` rellena todas las requeridas por el subject.
  - [ ] (Opcional) Mejorar `REMOTE_ADDR`, `SERVER_NAME`, `SERVER_PORT` a partir del socket/config real.

---

### 5. Static / uploads / DELETE

- [ ] **Uploads con `upload_store`**
  - [ ] Integrar `handleUpload` (ahora está comentado en `StaticPathHandler.cpp`).
  - [ ] Probar subida con formularios HTML puros (POST).

- [x] **DELETE estático**
  - [x] Soportado en `handleRegularFile` (unlink + errores).

- [x] **Doble check index CGI**
  - [x] Si el `index` es CGI, no servirlo como estático; redirigir para que pase por flujo CGI.

---

### 6. Frontend (HTML estático)

- [x] **`index.html` moderno**
  - [x] Página principal con explicación de rutas y tests manuales.
- [x] **`about.html`**
  - [x] Explica el proyecto y funcionalidades.
- [x] **Páginas de error 404/500**
  - [x] HTML consistente con el estilo del sitio.
- [ ] **Página sencilla de upload/DELETE solo con HTML**
  - [ ] Formularios `<form method="POST">` para upload.
  - [ ] Explicación textual de que DELETE no se puede hacer solo con HTML (usar curl).

---

### 7. Config / tests de configuración

- [ ] **Revisar `configs/default.conf` y ejemplos**
  - [ ] Que haya `cgi_handler` coherente con los scripts de `www/cgi-bin`.
  - [ ] Que `error_page` apunte a los HTML reales (404, 500…).

- [ ] **Tests manuales de configuración**
  - [ ] Probar servidor en ambos puertos configurados (por ejemplo 8080/8081).
  - [ ] Comprobar autoindex en las rutas que lo tengan activado.

---

### 8. Otros (refactor / limpieza)

- [ ] **Comentarios y docs**
  - [x] `CGI_Cliente_Plan.md`.
  - [x] `CGI_Body_y_Tamano.md`.
  - [x] `CGI_Variables_Entorno.md`.
  - [ ] Mantener estos docs sincronizados con el código a medida que se cambie algo.

