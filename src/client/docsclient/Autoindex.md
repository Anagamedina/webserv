# Autoindex (que es y como implementarlo)

## Que es
Autoindex es el listado automatico de archivos de un directorio cuando:
- la URL apunta a un directorio
- no existe un `index.html` (o index configurado)
- la configuracion tiene `autoindex on`

En ese caso, el servidor genera una pagina HTML con los archivos del directorio.

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
