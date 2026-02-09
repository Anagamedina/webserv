# Análisis de Configuración y Mejoras Propuestas

## Resumen de Hallazgos

### ❌ Errores Críticos (Bugs)
- **`LocationConfig::isMethodAllowed`**: Hay un error de lógica. La función tiene un `return false` prematuro que impide que el bucle de comprobación de métodos se ejecute correctamente. Actualmente, solo permite `GET` y `HEAD` e ignora lo que diga el archivo `.conf`.
- **Validación de Autoindex**: En `ConfigParser.cpp`, la condición `val != "on" || val != "off"` siempre es verdadera (lógicamente), lo que causa que cualquier valor (incluso "on" u "off") lance una excepción.

### ⚠️ Validaciones Faltantes
- **Directivas Duplicadas**: Si pones dos veces `listen` o `root` en un mismo bloque, el programa no avisa; simplemente usa el último valor, lo cual puede causar confusiones.
- **Punto y Coma (Strictness)**: El parsing actual es un poco permisivo con los `;`. Sería mejor validar estrictamente que cada directiva termine con uno para evitar comportamientos inesperados.
- **Métodos HTTP**: No se valida si los métodos en `limit_except` o `methods` son válidos (ej. `GET`, `POST`, `DELETE`). Se aceptan strings arbitrarios.
- **Formato de IP**: La directiva `listen` acepta cualquier string como host sin validar si es una IP válida o un nombre de dominio razonable.

---

## Recomendaciones Técnicas

### 1. Corregir Errores Lógicos
- Arreglar el bucle en `isMethodAllowed`.
- Cambiar el `||` por `&&` en la validación de `autoindex` (o usar una lógica más clara).

### 2. Control de Duplicados
- Usar un set o mapa temporal durante el parseo de cada bloque para asegurar que una directiva no se repita.

### 3. Mejorar ConfigUtils
- Añadir una función `isValidIp` para validar direcciones.
- Añadir un listado de métodos soportados para validarlos durante el parsing.

---

## Cambios Propuestos

### [Configuración]

#### [MODIFY] [ConfigParser.cpp](file:///home/daruuu/CLionProjects/webserv-fork/src/config/ConfigParser.cpp)
- Corregir lógica de `autoindex`.
- Implementar detección de directivas duplicadas.
- Validación más estricta de tokens.

#### [MODIFY] [LocationConfig.cpp](file:///home/daruuu/CLionProjects/webserv-fork/src/config/LocationConfig.cpp)
- Corregir `isMethodAllowed`.

#### [MODIFY] [ConfigUtils.cpp](file:///home/daruuu/CLionProjects/webserv-fork/src/config/ConfigUtils.cpp)
- Añadir utilidades de validación de IP y métodos.

## Plan de Verificación

### Pruebas Automatizadas
- Crear un archivo `.conf` con casos de error:
    - Autoindex con valores raros.
    - IPs mal formadas.
    - Métodos no soportados.

### Verificación Manual
- Ejecutar el servidor y comprobar que los mensajes de error son claros y específicos (especificando qué directiva falló).
