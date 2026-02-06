# Apuntes: RequestProcessor (explicado a mi manera)


## Idea principal
RequestProcessor es el cerebro del servidor: recibe un HttpRequest valido y la
configuracion, y decide que HttpResponse crear.

## 1) Matching (emparejar reglas)
Primero se decide que reglas aplicar segun la config:

- Primero por puerto (obligatorio).
- Luego, si hay varios en ese puerto, se usa Host como extra (virtual host).
- Location: se compara la URI con los bloques location para saber:
  - root o alias
  - metodos permitidos
  - si hay CGI
  - si hay redireccion

## 2) Validaciones (filtros)
Antes de procesar, se valida:

- Metodo permitido (si no, 405 Method Not Allowed).
- Tamano del body (si supera client_max_body_size, 413 Payload Too Large).
- Redireccion (si hay return 301/302, se responde y termina).

## 3) Resolver path real
Se convierte la URI en una ruta real del sistema:

root + URI = path real
Ejemplo: /www + /img/cat.jpg => /www/img/cat.jpg

## 4) Decidir el camino
Aqui se elige que tipo de respuesta construir.

### A) Contenido estatico
Si es archivo o directorio normal:

- Archivo: comprobar existe y permisos, leerlo y poner MIME type.
- Directorio: buscar index (index.html, etc).
  - Si no hay index y autoindex on, generar listado en HTML.
  - Si no, responder 403 o 404.

Notas extra para estatico:
- Unir config.root + request.getPath().
- Verificar si el archivo existe (access() o stat()).
- Si existe, leer el archivo y meterlo en response.setBody().
- Si no existe, responder 404.

### B) CGI (dinamico)
Si la extension o la location indica CGI:

- Preparar variables de entorno (QUERY_STRING, REQUEST_METHOD, etc).
- fork + execve para ejecutar el script.
- Leer la salida del CGI (headers + body).
- Si falla, responder 500.

### C) Acciones especiales (segun config)
- DELETE: borrar archivo con unlink() => 204 o 200.
- POST upload: guardar body en upload_store => 201 Created.

## 5) Construccion final de respuesta
Siempre se termina creando un HttpResponse completo:

- Status line (200, 404, 500, etc).
- Headers (Content-Length, Content-Type, Connection, Date).
- Body (archivo, listado, CGI o error).

## 5.b) Respuesta estatica (pasos practicos)
1) Resolver path real: root/alias + URI.
2) Comprobar existencia/permisos (stat/access).
3) Si es directorio: buscar index o autoindex.
4) Leer archivo en binario -> std::vector<char>.
5) Rellenar HttpResponse: status, content-type, body, connection.

## 6) Serializar respuesta
- HttpResponse::serialize() convierte la respuesta en bytes.
- Esos bytes se guardan en _outBuffer.
- El estado del cliente pasa a STATE_WRITING_RESPONSE.

## 7) Evento EPOLLOUT
- El server solo activa EPOLLOUT si needsWrite() == true.
- Eso significa: hay datos pendientes en _outBuffer.

## 8) Client::handleWrite()
- Se hace una sola llamada a send() por evento.
- Si se envian bytes: se recorta _outBuffer.
- Si falla (retorna < 0): estado STATE_CLOSED.

## 9) Fin de respuesta
Cuando _outBuffer queda vacio:

- Si la conexion es "close" -> STATE_CLOSED.
- Si es keep-alive -> reset del parser y volver a STATE_IDLE.

## Resumen de consejos aplicados
Apuntes extra sobre el flujo correcto (lo que nos recomendaron y ya corregimos):

- Errores primero: si el parseo falla, responder 400 y salir.
- Validaciones logicas antes de tocar disco:
  - Redirect (301/302) primero.
  - Metodo permitido (si no, 405).
  - Body size (si excede, 413).
- Directorios:
  - Si es directorio, primero buscar index.
  - Si no hay index y autoindex esta ON, generar listado.
  - Si falla todo, devolver 403.
- Estáticos:
  - GET: leer y devolver archivo.
  - DELETE: borrar con unlink.
  - POST: normalmente no permitido en estaticos (salvo uploads/CGI).

## Pasos actuales del flujo (y por qué)1) Inicialización  
   - status/body por defecto  
   - shouldClose según Connection  
   Por qué: necesitamos un estado base y saber si cerrar la conexión.2) ServerConfig por puerto/IP  
   - match por puerto → elegir ServerConfig correcto  
   - si no hay match → usar el primero  
   Por qué: es obligatorio servir contenido distinto por puerto.3) Location match  
   - usar server.getLocations() y la URI  
   - elegir la LocationConfig que mejor encaja  
   Por qué: cada location define reglas específicas.4) Validaciones  
   - método permitido  
   - body size  
   - redirect  
   Por qué: son reglas lógicas; si falla aquí, no tocamos disco.

5) Resolver path real  
   - root/alias + uri → path físico  
   Por qué: necesitamos la ruta real para acceder al filesystem.

6) Decidir respuesta  
   - estático (leer archivo)  
   - CGI (delegar a Carles)  
   - errores (403/404/405/413)  
   Por qué: define la respuesta final según tipo de recurso.

7) Construir HttpResponse  
   - status, headers, body  
   Por qué: el Client solo serializa y envía.