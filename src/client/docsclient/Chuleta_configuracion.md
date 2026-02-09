# Chuleta: Configuracion (ServerConfig / Location)

Esta chuleta resume como entender la configuracion y como se relaciona con el
RequestProcessor.

## 1) Estructura general del archivo .conf
Un archivo puede tener varios bloques `server { ... }`:

- Cada `server` es un **ServerConfig**.
- Dentro de un `server` hay varios `location { ... }`.

Ejemplo mental:
```
server { ... }   -> ServerConfig
server { ... }   -> ServerConfig
```

## 2) ServerConfig (global de un server)
Contiene cosas generales del servidor:
- Puerto (`listen`)
- Host (`host`)
- Server name (`server_name`)
- Limite de body (`max_body_size`)
- Error pages (`error_page`)
- Lista de locations

**Idea:** ServerConfig es lo "global" para ese server.

## 3) LocationConfig (reglas por ruta)
Cada `location` define reglas para una parte de la URL:
- Path (ej: `/img/`)
- Root / alias
- Index
- Autoindex
- Metodos permitidos
- CGI (si aplica)
- Upload (si aplica)

**Idea:** LocationConfig es lo "especifico" por ruta.

## 4) Matching (como se usa)
Paso a paso en RequestProcessor:
1. Elegir ServerConfig por `Host`
2. Elegir LocationConfig por URI
3. Aplicar reglas de location (metodos, root, CGI, etc)

## 5) Resolver path real (root + URI)
Despues del matching, se convierte la URI en ruta real del disco:

- Usando `root`: `root + uri`
- Si hay `alias`, se reemplaza el prefijo de la URI por el alias.

Ejemplo:
- root = /var/www
- uri = /img/logo.png
-> path real = /var/www/img/logo.png

## 5) Resumen rapido
- **ServerConfig = global**
- **LocationConfig = por ruta**
- **Vector<ServerConfig> = todos los servers del archivo**

