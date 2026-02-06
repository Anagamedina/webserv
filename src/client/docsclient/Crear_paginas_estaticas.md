# Crear paginas estaticas (pasos y como implementarlo)

## Objetivo
Tener paginas HTML (index, errores, etc.) que el servidor pueda servir desde el
directorio `www/`.

---

## Pasos recomendados

1) **Definir el directorio raiz (root)**
   - Por defecto: `./www`
   - En config: `root` del `ServerConfig` o `LocationConfig`

2) **Crear estructura de archivos**
   - `www/index.html` (pagina principal)
   - `www/about.html` (pagina extra)
   - `www/error_pages/404.html`
   - `www/error_pages/500.html`

3) **Actualizar config**
   - `root ./www;`
   - `index index.html;`
   - (opcional) `error_page 404 /error_pages/404.html;`

4) **Asegurar lectura en RequestProcessor**
   - Resolver path real: `root + uri`
   - `stat()` para saber si existe
   - Si es archivo regular → leer y devolver
   - Si es directorio → buscar index

5) **Contenido-Type correcto**
   - `HttpResponse::setContentType(path)` ya elige MIME por extension

---

## Como implementarlo en el codigo

### A) Crear paginas (archivos fisicos)
1. Crea los archivos en `www/`
2. Escribe HTML simple (titulos, parrafos, enlaces)

### B) Servir paginas desde RequestProcessor
Ya tenemos este flujo:
- `resolvePath()` -> obtiene ruta real
- `handleStaticPath()` -> usa `stat`, busca index, lee el archivo

Solo necesitas que la config tenga `root` e `index` correctos.

---

## Check rapido
- [ ] Existe `www/index.html`
- [ ] `root` apunta a `./www`
- [ ] `index` incluye `index.html`
- [ ] `RequestProcessor` lee el archivo si es GET

---

## Ejemplo minimo de HTML

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Webserv</title>
  </head>
  <body>
    <h1>Hola Webserv</h1>
    <p>Pagina estatica funcionando.</p>
  </body>
</html>
```
