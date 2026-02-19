# Análisis de Cumplimiento: src/config

Este documento detalla las funciones utilizadas en la carpeta `src/config` que podrían incumplir las normas del subject.

## 1. Funciones Prohibidas (Syscalls y C-Lib)

| Función | Ubicación | Motivo |
| :--- | :--- | :--- |
| **`mkdir`** | `ConfigUtils.cpp:452` | **Crítico**. No figura en la lista permitida. Se usa en `ensureUploadStorePath`. |
| **`strtol`** | `ConfigUtils.cpp:173, 210` | No figura en la lista. Se usa para convertir cadenas a números con detección de errores. |
| **`std::isdigit`, `std::isalnum`, etc.** | `ConfigUtils.cpp` | Funciones de `<cctype>`. Aunque son estándar, no están explícitamente en tu lista. |

## 2. Bibliotecas de C++ vs. Syscalls de C

El subject permite `open`, `read`, `write` y `close`. Sin embargo, en `src/config` se utiliza la biblioteca estándar de C++ para I/O:

- **`std::ifstream` / `std::ofstream`**: Utilizados en `ConfigParser.cpp` y `ConfigUtils.cpp` para leer el archivo de configuración y escribir logs.
- **`std::getline`**: Utilizado para leer el archivo línea por línea.
- **`std::stringstream`**: Utilizado para manipulación de cadenas.

> [!WARNING]
> Si tu subject exige el uso estricto de los syscalls de C para toda operación de I/O, el uso de `<fstream>` es un incumplimiento. Deberías reemplazarlo por `open()`, `read()` y un buffer manual.

## 3. Estándar C++98

El código es compatible con C++98 en su mayoría:
- Uso de `std::vector`, `std::map`, `std::string`.
- Uso de excepciones (`try/catch`).
- **Punto a revisar**: El uso de `__DATE__` y `__TIME__` en `ConfigUtils.cpp:480` es estándar de C, pero a veces se desaconseja por no ser determinista en el build.

## Recomendaciones para src/config

1.  **Eliminar `mkdir`**: Si no está permitido, no puedes crear el directorio de subida automáticamente. Debes asumir que existe o lanzar un error si no está.
2.  **Reemplazar `strtol`**: Implementar una función propia para convertir `string` a `long` que verifique desbordamientos y caracteres no válidos manualmente.
3.  **Confirmar `fstream`**: Verifica si se te permite usar la biblioteca estándar de C++ para archivos. Si no, tendrás que refactorizar el `preprocessConfigFile` para usar `read()` con un buffer.
