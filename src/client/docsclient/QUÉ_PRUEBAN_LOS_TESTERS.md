# ¿Qué prueban los testers de webserv?

Resumen para entender qué validaciones del parseo pueden estar fallando.

---

## 1. Testers en el proyecto

| Archivo        | Ubicación        | Qué hace                                                    |
|----------------|------------------|-------------------------------------------------------------|
| `tester`       | `testers/`       | Binario (probablemente tester oficial 42 o similar)         |
| `ubuntu_tester`| `testers/`       | Versión para Ubuntu                                        |
| `cgi_tester`   | `testers/`       | Pruebas de CGI                                              |
| Tests unitarios| `tests/unit/config/` | `test_config_parser.cpp`, `test_validations.cpp`     |

---

## 2. Qué suelen probar los testers de config (parseo)

### A) Configs **inválidas** → el servidor debe **fallar** (exit error o excepción)

- **Falta punto y coma** en directivas  
  Ejemplo: `server_name example.com` (sin `;`)  
  - `config/test-cases-error/invalid_syntax.conf` tiene este caso.  
  - Si el parser **acepta** esto, muchos testers lo marcan como fallo.

- **Llaves desequilibradas**  
  Ejemplo: `server {` sin `}` o `}` de más.  
  - Tenéis `validateBalancedBrackets()` → debería detectarlo.

- **Puerto inválido** (0, negativo, >65535)  
  - `ServerConfig::setPort()` ya valida y lanza excepción.

- **Código de error inválido** en `error_page` (ej. 99, 600)  
  - `ServerConfig::addErrorPage()` valida rango 100–599.

- **Método HTTP inválido** en `allow_methods` (ej. PUT, get minúscula)  
  - `ConfigParser` usa `isValidHttpMethod()` → solo GET, POST, DELETE.

- **Ruta de location inválida** (sin `/` inicial, `//`, etc.)  
  - `isValidLocationPath()` se usa en `parseLocationBlock`.

- **Host/IP inválido** en `listen` (ej. `256.1.1.1`)  
  - `parseListen` llama a `isValidHost()`.

### B) Configs **válidas** → el servidor debe **arrancar**

- Configs en `config/test-cases-valid/` deben parsear sin errores.

---

## 3. Validación que podría faltar: punto y coma obligatorio

En Nginx, **todas** las directivas deben terminar con `;`.  
Ejemplo inválido:

```nginx
server_name example.com    # ← Falta ;
```

Actualmente `removeSemicolon()` solo **quita** el `;` si existe; no **comprueba** que exista. Si una línea no tiene `;`, el parser puede aceptarla igual.

**Propuesta:** validar que el último token de la línea contenga `;` antes de procesar (o que la directiva termine en `;`), y lanzar `ConfigException` si falta.

---

## 4. Cómo ver qué falla exactamente

### Opción 1: Tests unitarios (Catch2)

```bash
# Desde la raíz del proyecto (ajustar si usáis otro build)
cd build && make && ./unit_tests  # o el nombre del ejecutable de tests
```

Ahí veréis qué tests de `test_validations.cpp` y `test_config_parser.cpp` pasan o fallan.

### Opción 2: Probar configs inválidas a mano

```bash
./webserver config/test-cases-error/invalid_syntax.conf
```

- Si el servidor **arranca** con esa config → el parser está aceptando algo que debería rechazar.
- Si **sale con error** → bien.

### Opción 3: Preguntar a tu compañero

Que ejecute el tester con **verbose** (si lo permite) o que anote el **mensaje de error** o el **nombre del test** que falla. Con eso se puede localizar la validación concreta.

---

## 5. Checklist rápido de validaciones

| Validación                 | ¿Implementada?        | Dónde                          |
|---------------------------|------------------------|--------------------------------|
| Puerto 1–65535            | Sí                     | `ServerConfig::setPort()`      |
| Error codes 100–599       | Sí                     | `ServerConfig::addErrorPage()`  |
| Host/IP en listen         | Sí                     | `parseListen` → `isValidHost()`|
| Métodos GET/POST/DELETE   | Sí                     | `isValidHttpMethod()`          |
| Ruta de location          | Sí                     | `isValidLocationPath()`        |
| Llaves equilibradas       | Sí                     | `validateBalancedBrackets()`   |
| Punto y coma obligatorio  | Probablemente no       | No hay comprobación explícita  |
| parseSize (1K, 1M, etc.)  | Sí                     | `ConfigUtils::parseSize()`     |

---

## 6. Próximo paso recomendado

1. Que tu compañero ejecute el tester y anote **qué test falla** (nombre o mensaje).
2. Probar manualmente:  
   `./webserver config/test-cases-error/invalid_syntax.conf`  
   y ver si arranca o falla.
3. Si hace falta, añadir validación de **punto y coma obligatorio** en las directivas.
