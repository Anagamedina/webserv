# Apuntes de repaso: Configuración (Config Parser)

> Resumen para entender el parseo de la configuración tipo Nginx.

---

## 1. ¿Para qué sirve la config?

El archivo `.conf` define cómo se comporta cada **server** y sus **locations**:

- Puerto y host de escucha
- Rutas, root, index, métodos permitidos
- Páginas de error personalizadas
- CGI, redirecciones, upload, autoindex, etc.

---

## 2. Estructura tipo Nginx

```nginx
server {
    listen 8080;
    server_name example.com;
    root ./www;
    index index.html;
    client_max_body_size 1M;
    error_page 404 /404.html;
    error_page 500 502 503 /50x.html;

    location / {
        allow_methods GET POST DELETE;
        index index.html;
    }

    location /cgi-bin {
        cgi .py /usr/bin/python3;
        cgi .php /usr/bin/php;
    }

    location /upload {
        upload_store ./uploads;
        allow_methods POST;
    }
}
```

---

## 3. Archivos principales

| Archivo | Responsabilidad |
|---------|-----------------|
| `ConfigParser.cpp/hpp` | Lee el .conf, valida, extrae bloques server, parsea directivas |
| `ServerConfig.cpp/hpp` | Datos de un bloque `server { }` |
| `LocationConfig.cpp/hpp` | Datos de un bloque `location /path { }` |
| `ConfigUtils.cpp/hpp` | Utilidades (parseSize, validaciones, etc.) |
| `ConfigException.cpp/hpp` | Excepciones con mensaje claro |

---

## 4. Flujo de parseo (ConfigParser::parse)

```
1. Validar extensión .conf
2. Validar que se pueda abrir el archivo
3. preprocessConfigFile() → limpiar comentarios, normalizar
4. validateBalancedBrackets() → comprobar { }
5. loadServerBlocks() → extraer cada bloque "server { ... }"
6. parseAllServerBlocks() → para cada bloque:
   └─> parseSingleServerBlock()
         ├─> parseListen, parseRoot, parseIndex, parseServerName
         ├─> parseMaxSizeBody, parseErrorPage
         └─> parseLocationBlock (para cada location)
               └─> parseRoot, parseCgi, parseReturn, parseUploadBonus, etc.
```

---

## 5. ServerConfig: campos

| Campo | Directiva | Significado |
|-------|-----------|-------------|
| `listen_port` | `listen 8080` | Puerto de escucha |
| `host_address` | `host 127.0.0.1` | IP/host |
| `server_name` | `server_name example.com` | Nombre del virtual host |
| `root` | `root ./www` | Directorio raíz por defecto |
| `indexes` | `index index.html` | Archivos índice |
| `max_body_size` | `client_max_body_size 1M` | Límite de body (bytes) |
| `error_pages` | `error_page 404 /404.html` | Mapa código → ruta |
| `locations` | `location / { }` | Vector de LocationConfig |
| `autoindex` | `autoindex on/off` | Listar directorios |
| `redirect_*` | `return 301 /new` | Redirección |

---

## 6. LocationConfig: campos

| Campo | Directiva | Significado |
|-------|-----------|-------------|
| `path` | `location /upload` | Ruta que matchea |
| `root` | `root ./www` | Root para esta location |
| `indexes` | `index a.html b.html` | Índices |
| `allowed_methods` | `allow_methods GET POST` | Métodos permitidos |
| `autoindex` | `autoindex on` | Listado de directorios |
| `upload_store` | `upload_store ./uploads` | Carpeta para POST upload |
| `redirect_code/url` | `return 301 /x` | Redirección |
| `cgi_handlers` | `cgi .py /usr/bin/python3` | Extensión → intérprete |

---

## 7. Validaciones importantes

| Validación | Dónde | Qué comprueba |
|------------|-------|---------------|
| Extensión .conf | `validateFileExtension()` | Termina en `.conf` |
| Permisos | `validateFilePermissions()` | Se puede abrir el archivo |
| Llaves | `validateBalancedBrackets()` | Número correcto de `{` y `}` |
| Puerto | `ServerConfig::setPort()` | 1–65535 |
| Códigos error | `addErrorPage()` | 100–599 |
| Host | `isValidHost()` en parseListen | IP o hostname válido |
| Métodos | `isValidHttpMethod()` | GET, POST, DELETE |
| Ruta location | `isValidLocationPath()` | Comienza con `/`, sin `//` |

---

## 8. Matching en runtime

Cuando llega una petición, se usa la config así:

1. **Server:** Por puerto (`listenPort`). Si varios comparten puerto, por `Host` (virtual host).
2. **Location:** Se compara la URI con cada `location /path`. Se elige la de path más largo que coincida.

```cpp
selectServerByPort(port, configs)  → ServerConfig*
matchLocation(server, uri)        → LocationConfig*
resolvePath(server, location, uri) → path real en disco
```

---

## 9. Directivas y funciones de parseo

| Directiva | Función | Ejemplo |
|-----------|---------|---------|
| `listen` | `parseListen()` | `listen 8080;` |
| `root` | `parseRoot()` | `root ./www;` |
| `index` | `parseIndex()` | `index a.html b.html;` |
| `server_name` | `parseServerName()` | `server_name x.com;` |
| `client_max_body_size` | `parseMaxSizeBody()` | `1M`, `1k` |
| `error_page` | `parseErrorPage()` | `error_page 404 /404.html;` |
| `allow_methods` | `parseLocationBlock` | `GET POST DELETE` |
| `cgi` | `parseCgi()` | `cgi .py /usr/bin/python3;` |
| `return` | `parseReturn()` | `return 301 /new;` |
| `upload_store` | `parseUploadBonus()` | `upload_store ./uploads;` |

---

## 10. Posibles errores de testers

- **Falta `;`** al final de directivas → muchos testers lo marcan como inválido.
- **Llaves desequilibradas** → `validateBalancedBrackets`.
- **Puerto/códigos fuera de rango** → validaciones en setters.

---

## 11. Checklist de repaso

- [ ] Entiendo la estructura server/location tipo Nginx
- [ ] Conozco el flujo de ConfigParser::parse
- [ ] Sé qué guarda ServerConfig y LocationConfig
- [ ] Entiendo el matching: puerto → server, URI → location
- [ ] Conozco las validaciones principales
- [ ] Sé resolver path real: root + URI
