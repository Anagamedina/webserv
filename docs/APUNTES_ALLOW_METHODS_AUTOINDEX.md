# Métodos HTTP permitidos y Autoindex

Apuntes sobre `allow_methods`, validación de métodos, respuestas 405, y el funcionamiento del autoindex.

---

## 1. allow_methods vs limit_except (NGINX)

### NGINX real: limit_except (lógica negativa)

Funciona mediante bloques anidados. Limita los métodos que **NO** están en la lista:

```nginx
location /uploads {
    limit_except GET {
        deny all;
    }
}
```

**Significado:** "Para todo lo que no sea GET, deniega el acceso". Solo GET (y HEAD implícito) están permitidos.

### Nuestro webserv: allow_methods (lógica positiva)

El subject pide una "lista de métodos HTTP aceptados". Simplificamos a una lista positiva:

```nginx
location /uploads {
    allow_methods GET POST DELETE;
}
```

**Significado:** "Solo permito GET, POST y DELETE". Todo lo demás devuelve 405.

**Nota:** No soportamos `limit_except`. Solo usamos `methods` o `allow_methods`.

---

## 2. Paso A: Archivo de configuración

Para rutas de subida (ej. `/uploads`), asegúrate de incluir POST explícitamente:

```nginx
location /uploads {
    root ./www;
    autoindex on;
    allow_methods GET POST DELETE;
    upload_store ./www/uploads;
}
```

- **GET** — listar / descargar archivos
- **POST** — subir archivos (requiere `upload_store`)
- **DELETE** — borrar archivos

---

## 3. Paso B: Validación en RequestProcessor

En `validateLocation` se comprueba si el método está permitido:

```cpp
if (!location->isMethodAllowed(methodToString(request.getMethod())))
    return 405;
```

Si el método no está en la lista → 405 Method Not Allowed.

---

## 4. Paso C: Limpieza del buffer ("Pending")

Cuando envías 405 inmediatamente, el cliente puede seguir enviando el body (ej. un archivo grande). El navegador puede mostrar "Pending" indefinidamente.

**Solución:** Usar `Connection: close` en la respuesta 405. Así:
1. El servidor cierra la conexión tras enviar el 405
2. El navegador recibe la respuesta y deja de esperar
3. No hace falta leer/descartar el body; al cerrar la conexión el cliente deja de enviar

En el código, `buildErrorResponse` recibe `shouldClose=true` para errores de validación (incluido 405), lo que fija `Connection: close` en `fillBaseResponse`.

---

## 5. Autoindex: resolución de rutas

### Cómo funciona resolvePath

`path = root + uri`

- **root** — directorio base (ej. `./www`)
- **uri** — ruta de la petición (ej. `/uploads/`)

**Ejemplo:**
- `root ./www` + `uri /uploads/` → `./www/uploads/`

### Error común: root mal configurado

```nginx
# ❌ Incorrecto
location /uploads {
    root ./www/uploads;
}
# path = ./www/uploads + /uploads = ./www/uploads/uploads (no existe)
```

```nginx
# ✅ Correcto
location /uploads {
    root ./www;
}
# path = ./www + /uploads = ./www/uploads
```

### Directorios necesarios

Para que el autoindex funcione:
1. El directorio físico debe existir (ej. `www/uploads/`, `www/test_dir/`)
2. El `root` debe ser el padre: `root ./www` para que `/uploads` mapee a `./www/uploads`

---

## 6. Cómo probar el autoindex

1. Crear los directorios: `www/uploads/`, `www/test_dir/`
2. Añadir algún archivo para que la lista no esté vacía
3. Configurar `autoindex on` en la location
4. Acceder con la **barra final**: `http://localhost:8080/uploads/`, `http://localhost:8080/test_dir/`

---

## 7. Ejemplo completo (default.conf)

```nginx
# Ruta raíz
location / {
    root ./www;
    index index.html index.htm;
    autoindex off;
}

# Subida de archivos con autoindex
location /uploads {
    root ./www;
    autoindex on;
    allow_methods GET POST DELETE;
}

# Otro directorio con autoindex
location /test_dir {
    root ./www;
    autoindex on;
}
```

---

## Resumen rápido

| Concepto | Detalle |
|----------|---------|
| `allow_methods` | Lista positiva: solo los listados están permitidos |
| `limit_except` | No soportado; usar `allow_methods` |
| 405 | `Connection: close` para evitar "Pending" |
| `root` | Directorio base; la URI se añade al final |
| Autoindex | Requiere directorio existente + `autoindex on` |
| URL directorio | Usar barra final: `/uploads/` |
