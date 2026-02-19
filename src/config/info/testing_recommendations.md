# Recomendaciones de Testing para Configuración (.conf)

Basado en el estado actual del proyecto `webserv`, estas son las estrategias recomendadas para asegurar que el parseo y validación de la configuración sean robustos.

## 1. Robustez Estructural (Fuzzing Manual)
El parser debe ser capaz de manejar errores de sintaxis comunes sin crashear.
- **Puntos y coma:** Probar directivas sin `;` al final.
- **Bloques mal formados:** Llaves `{` sin cerrar o llaves extra `}`.
- **Directivas desconocidas:** Asegurar que el parser detecta y reporta nombres de directivas inexistentes.
- **Contexto inválido:** Intentar poner directivas de nivel `server` (como `listen`) dentro de un bloque `location`.
- **Comentarios y espacios:** Verificar que `# comentarios` en mitad de línea y múltiples espacios/tabuladores no alteran el resultado.

## 2. Herencia de Directivas (Lógica de Negocio)
Es fundamental verificar que la jerarquía de configuración funciona según lo esperado:
- **Default -> Server:** Si una directiva no está en el `server`, debe tomar el valor por defecto.
- **Server -> Location:** Las `location` deben heredar valores como `root`, `index` y `client_max_body_size` de su bloque `server` padre.
- **Sobrescritura:** Una directiva definida explícitamente en `location` debe anular la heredada del `server`.

## 3. Suite de Casos de Error (Integración)
Crear una colección de archivos `.conf` pequeños que representen "casos límite":
- **Puertos inválidos:** `0`, `65536`, `-1`, `abc`.
- **Hosts inválidos:** `256.0.0.1`, `-localhost`, `host..name`.
- **Rutas de Location:** Asegurar que siempre empiecen por `/`.
- **Métodos permitidos:** Probar métodos fuera de la whitelist (ej. `PUT`, `PATCH`).
- **Tamaños de cuerpo:** `3G` (excede límite de 2GB), `-100`, `10KK`.

## 4. Casos Especiales del Bonus
- **Directiva Return:** 
  - `return 301 http://ejemplo.com;` (2 parámetros).
  - `return 404;` (1 parámetro).
- **CGI:**
  - Múltiples extensiones `.py` `.php` en una misma `location`.
  - Extensiones sin intérprete asociado.

## Herramientas Recomendadas
- **Catch2:** Continuar expandiendo `test_config_integration.cpp`.
- **Scripts de Bash:** Un script simple que ejecute el binario con `--test` (si implementas un modo de solo testeo) sobre una carpeta de archivos "bad_config/*.conf".
