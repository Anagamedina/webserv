# Parseo - Tareas Pendientes y Mejoras

## 1. Soporte para CGI ⚙️
*   **Qué falta:** Directivas para manejar CGI (como `.php` o `.py`).
*   **Por qué es importante:** Obligatorio para ejecutar scripts en el subject.
*   **Ejemplo:**
    ```nginx
    location ~ \.php$ {
        fastcgi_pass 127.0.0.1:9000;
        cgi_path /usr/bin/php-cgi; 
    }
    ```

## 2. Parseo Robusto de `listen` 🎧
*   **Qué falta:** Actualmente `atoi` asume solo un puerto.
*   **Problema:** Nginx permite `listen 127.0.0.1:8080;` o `listen 80;`.
*   **Solución:** Separar IP y PUERTO si vienen juntos antes de convertir.

## 3. Validaciones de Errores (Safety) 🛡️
*   **Qué falta:** Uso inseguro de `atoi`.
*   **Riesgo:** `client_max_body_size patata;` devuelve 0.
*   **Solución:** Usar una función auxiliar con `strtol` que lance excepción si no es un número válido.

## 4. Directivas `alias` vs `root` 📂
*   **Qué falta:** Implementar `alias`.
*   **Diferencia:** `alias` reemplaza la ruta completa, `root` concatena.
*   **Solución:** Añadir `alias` a `LocationConfig` y al parser.

## 5. `limit_except` (Métodos permitidos) 🚫
*   **Qué falta:** Asegurar que `allow_methods` bloquea explícitamente todo lo que no esté listado (Whitelist estricta).

## 6. Locations Anidados 📦
*   **Qué falta:** El parser actual puede fallar con bloques dentro de bloques.
*   **Ejemplo:**
    ```nginx
    location / {
        location /images { ... }
    }
    ```
