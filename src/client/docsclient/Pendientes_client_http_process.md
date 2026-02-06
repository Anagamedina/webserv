# Pendientes (Client / HTTP / RequestProcessor)

## Client (mi parte)
- Me falta integrar CGI real: crear el proceso, registrar los pipes en epoll y leer la salida.
- El timeout ahora es fijo; me falta hacerlo configurable por config si quiero.
- El keep-alive es básico; podría mejorar el manejo de requests parciales o grandes.
- El manejo de errores de socket es simple (cierro sin distinguir casos).

## HTTP / HttpResponse
- Falta decidir si quiero añadir el header Date.
- Podría ampliar el mapa de Content-Type para más extensiones.
- No hay soporte de chunked en responses (si llego a necesitarlo).
- Revisar consistencia de headers de error.

## RequestProcessor
- Falta usar `error_page` de config para servir páginas de error reales.
- Falta integrar uploads (POST a upload_store).
- Falta integración real de CGI (llamar a CgiExecutor/Handler).
- La resolución root/alias es básica; podría ajustarla si uso alias.
- Autoindex está, pero es simple (sin estilo ni permisos).
