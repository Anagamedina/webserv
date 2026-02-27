# Análisis de Herencia y Prioridad de Configuración

La gestión de directivas repetidas entre los bloques `server` y `location` sigue un modelo de **"Location-wins"** (la localización siempre gana) con mecanismos específicos de **fallback** (retroceso) en tiempo de ejecución (replicando a como funciona Nginx).

## Resumen de Prioridades

La prioridad general es:
1. **Directiva en `location { ... }`** (Si está presente, se usa siempre).
2. **Directiva en `server { ... }`** (Funciona como fallback para algunas directivas).
3. **Valor por defecto** (Si no está en niguno de los dos).

---

## Detalle de Directivas Específicas

| Directiva | Prioridad / Comportamiento | Lógica de Implementación                                                                                                                              |
| :--- | :--- |:------------------------------------------------------------------------------------------------------------------------------------------------------|
| **`root`** | `location` > `server` > `./www` | Si `location` no tiene `root`, usa el del `server`. Si este también falta, usa `./www`.                                                               |
| **`index`** | `location` > `server` > `index.html` | Si `location` no tiene `index`, usa los del `server`. Si ambos están vacíos, busca `index.html`.                                                      |
| **`client_max_body_size`** | `location` > `server` | `location` siempre sobreescribe al `server`. Si no la pones en `location`, usará el valor por defecto (1MB) ignorando lo que pusieras en el `server`. |
| **`autoindex`** | Solo `location` | Actualmente solo comprobamos el valor en el bloque `location`. No hay fallback al `server`.                                                           |
| **`allow_methods`** | Solo `location` | Comprobamos exclusivamente en el bloque `location`. Si no hay metodos, el valor por defecto es solo `GET`.                                            |
| **`return`** | Solo `location` | La redirección se valida únicamente a nivel de `location`.                                                                                            |
| **`error_page`** | Solo `server` | Dado que [LocationConfig](../src/config/LocationConfig.hpp#25-26) no tiene este campo, se gestionan globalmente para todo el servidor.                |

---

## Análisis Técnico de la Gestión

### 1. Fase de Parseo ([ConfigParser.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp))
Durante el parseo, los objetos [ServerConfig](file:///home/daruuu/CLionProjects/webserv/src/config/ServerConfig.hpp#38-39) y [LocationConfig](file:///home/daruuu/CLionProjects/webserv/src/config/LocationConfig.hpp#25-26) se llenan de forma **independiente**:
- Cuando el parser encuentra un bloque `location`, crea un objeto [LocationConfig](file:///home/daruuu/CLionProjects/webserv/src/config/LocationConfig.hpp#25-26) **vacío**.
- **No se copian** los valores del `server` al `location` en esta fase. Cada objeto solo guarda lo que tiene escrito físicamente en su bloque.

### 2. Fase de Ejecución ([RequestProcessorUtils.cpp](file:///home/daruuu/CLionProjects/webserv/src/client/RequestProcessorUtils.cpp))
La verdadera "herencia" ocurre en tiempo de ejecución, cuando llega una petición:

- **`root` y `index`**: Tienen lógica de fallback explícita. Si el getter de `location` devuelve un campo vacío, el código busca el valor en el objeto `server`.
- **`client_max_body_size`**: El código de validación toma el valor del `server` y lo **reemplaza totalmente** si existe un objeto `location` (independientemente de si el usuario lo configuró o es el valor por defecto del constructor).

> [!IMPORTANT]
> **Nota sobre `client_max_body_size`**: Debido a que el constructor de [LocationConfig](file:///home/daruuu/CLionProjects/webserv/src/config/LocationConfig.hpp#25-26) inicializa este valor a 1MB por defecto, cualquier `server { client_max_body_size 100M; }` será ignorado en favor del 1MB por defecto si la petición cae en una `location` que no repita la directiva explícitamente.

---
