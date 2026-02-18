# Autoindex (que es y como implementarlo)

## Que es
Autoindex es el listado automatico de archivos de un directorio cuando:
- la URL apunta a un directorio
- no existe un `index.html` (o index configurado)
- la configuracion tiene `autoindex on`

En ese caso, el servidor genera una pagina HTML con los archivos del directorio.

---

## Cuando ve el usuario esta pagina (autoindex on vs off)

**Situación:** El usuario entra a una URL que es un **directorio**, por ejemplo:
- `http://localhost:8080/uploads/`
- `http://localhost:8080/images/`
- `http://localhost:8080/docs/`

En ese directorio **no hay** archivo índice (no hay `index.html`, ni el que pongas en `index`).

| Configuración   | Qué ve el usuario |
|-----------------|--------------------|
| **autoindex on**  | La página de listado: título "Index of /uploads/", enlaces a cada archivo y subcarpeta. Puede navegar como en un explorador de archivos. |
| **autoindex off** | **403 Forbidden**: página de error. El servidor no muestra el contenido del directorio; el usuario no puede listar archivos. |

**Finalidad:**
- **on**: útil para carpetas de descargas, galerías (`/images/`), `/uploads/`, documentación (`/docs/`) donde quieres que la gente pueda ver y abrir archivos sin tener que crear un `index.html` a mano.
- **off**: por seguridad o privacidad: no quieres que nadie liste el contenido del directorio, solo que acceda a archivos concretos si conocen la URL.

---

## Donde va en el flujo
1) Request -> match server -> match location  
2) Validaciones (metodo, body size, redirect)  
3) Resolver ruta real  
4) **Si es directorio**:
   - buscar index
   - si no hay index y autoindex on -> generar HTML

---

## Como lo implemente (resumen)

En `StaticPathHandler.cpp`:
- Si `S_ISDIR` y no hay index:
  - si `location->getAutoIndex()` es true:
    - abrir el directorio con `opendir`
    - leer entradas con `readdir`
    - generar HTML con links
    - devolver `Content-Type: text/html`

---

## Cosas importantes
- Se ignoran `.` y `..`
- Si el nombre es carpeta, se añade `/` al link
- El listado es basico, pero sirve para probar

---

## Ejemplo de salida

```html
<h1>Index of /docs/</h1>
<ul>
  <li><a href="/docs/file.txt">file.txt</a></li>
  <li><a href="/docs/images/">images/</a></li>
</ul>
```
