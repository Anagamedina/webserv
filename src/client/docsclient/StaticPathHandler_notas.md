# Notas: StaticPathHandler (resumen de logica)

## Lectura de archivos
- Se lee en binario para soportar contenido no‑texto (imagenes, PDFs, etc).
- Se guarda en `std::vector<char>`.

## Deteccion de tipo
- `getPathInfo()` usa `stat` para saber si es directorio o archivo regular.

## Directorios
- Si es directorio:
  - buscar index en `location->getIndexes()`
  - si no hay, usar `server.getIndex()`
  - si no hay, usar `index.html`
  - si existe index → leerlo
  - si no existe index y `autoindex` esta ON → generar listado HTML
  - si no → 403

## Archivos regulares
- `POST` → 405 (por defecto)
- `DELETE` → `unlink()` (si falla → 500)
- `GET` → leer archivo (si falla → 403)

## Errores
- `stat` falla → 404
- No es archivo regular → 403
