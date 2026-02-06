# Estrategia de Testing Unitario para ConfigParser

Plan completo para implementar tests unitarios usando **Catch2** en C++98 para el módulo de parsing de archivos de configuración tipo nginx.

## Análisis del Código Actual

### Funciones a Testear

Basándome en [ConfigParser.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.hpp), las funciones críticas son:

**Métodos Públicos:**
- [parse()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#64-99) - Función principal de parsing
- [getServerCount()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#54-58) - Retorna cantidad de servers

**Métodos Privados Críticos:**
- [ValidateFileExtension()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#100-115) - Valida extensión [.conf](file:///home/daruuu/CLionProjects/webserv/config/default.conf)
- [ValidateFilePermissions()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#116-124) - Valida permisos de lectura
- [ValidateCurlyBrackets()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#161-196) - **Recién corregido** - valida balance de `{}`
- [CleanFileConfig()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#125-160) - Limpia comentarios y espacios
- [RemoveComments()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#198-207) - Elimina líneas con `#`
- [TrimLine()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#253-276) - Elimina whitespace
- [NormalizeSpaces()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#317-331) - Normaliza espacios múltiples
- [extractServerBlocks()](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp#371-409) - Extrae bloques `server { }`

## Categorías de Tests Propuestos

### 1️⃣ **Tests de Validación de Brackets** (PRIORIDAD ALTA)

> [!IMPORTANT]
> Esta es la funcionalidad recién implementada y requiere testing exhaustivo.

**Casos a testear:**

| Caso | Input | Resultado Esperado | Descripción |
|------|-------|-------------------|-------------|
| Balanceados simples | `server { }` | `true` | Un par balanceado |
| Balanceados anidados | `server { location / { } }` | `true` | Anidamiento correcto |
| Más aperturas | `server { location { }` | `false` | Falta un `}` |
| Más cierres | `server { } }` | `false` | Sobra un `}` |
| Cierre prematuro | `} server {` | `false` | **Caso crítico** - el nuevo código debe detectar esto |
| Múltiples servers | `server { } server { }` | `true` | Múltiples bloques válidos |
| Vacío | `` | `true` | String vacío = válido |

**Enfoque de testing:**
- Crear archivos [.conf](file:///home/daruuu/CLionProjects/webserv/config/default.conf) de prueba en `tests/data/`
- Tests aislados que solo validen brackets sin parsear contenido completo

---

### 2️⃣ **Tests de Validación de Archivos** (PRIORIDAD ALTA)

**Casos a testear:**

| Función | Caso | Input | Resultado |
|---------|------|-------|-----------|
| `ValidateFileExtension()` | Extensión válida | `server.conf` | `true` |
| | Extensión inválida | `server.txt` | `false` |
| | Sin extensión | `server` | `false` |
| | Path muy corto | `a.co` | `false` |
| `ValidateFilePermissions()` | Archivo existente y legible | `valid.conf` | `true` |
| | Archivo no existente | `noexiste.conf` | `false` |
| | Archivo sin permisos | `noperm.conf` | `false` |

**Preparación:**
- Crear archivos de test en `tests/data/valid/` y `tests/data/invalid/`
- Para test de permisos: usar `chmod 000` en CI/test setup

---

### 3️⃣ **Tests de Limpieza de Contenido** (PRIORIDAD MEDIA)

**`RemoveComments()`:**

```cpp
Input:  "server { # comentario"
Output: "server { "

Input:  "# línea completa comentada"
Output: ""

Input:  "listen 8080; # port"
Output: "listen 8080; "
```

**`TrimLine()`:**

```cpp
Input:  "   server   "
Output: "server"

Input:  "\t\nlisten\r\n"
Output: "listen"

Input:  "   "
Output: ""
```

**`NormalizeSpaces()`:**

```cpp
Input:  "listen    8080"
Output: "listen 8080"

Input:  "server  {   location   /   }"
Output: "server { location / }"
```

**Enfoque:**
- Tests unitarios directos de funciones auxiliares
- No requieren archivos, solo strings

---

### 4️⃣ **Tests de Integración de Parsing** (PRIORIDAD MEDIA)

**Casos completos:**

| Escenario | Archivo | Validaciones |
|-----------|---------|--------------|
| Config simple válido | `simple_valid.conf` | `parse()` no lanza excepción, `getServerCount() == 1` |
| Multiple servers | `multi_server.conf` | `getServerCount() == 3` |
| Server con locations anidadas | `nested_locations.conf` | Extrae correctamente bloques |
| Config vacío | `empty.conf` | Lanza excepción o retorna 0 servers |
| Solo comentarios | `only_comments.conf` | Retorna 0 servers |
| Brackets mal balanceados | `unbalanced.conf` | Lanza `ConfigException` |
| Extensión incorrecta | `config.txt` | Lanza `ConfigException` |

---

### 5️⃣ **Tests de Extracción de Bloques** (PRIORIDAD BAJA)

**`extractServerBlocks()`:**

```cpp
// Input: "server { listen 80; } server { listen 8080; }"
// Expected: raw_server_blocks_.size() == 2
// Expected: servers_count_ == 2
```

**Casos edge:**
- Server sin location
- Server con múltiples locations
- Comentarios entre servers

---

## Estructura de Archivos de Test Propuesta

```
tests/
├── unit/
│   ├── CMakeLists.txt                    [EXISTE]
│   ├── main_test.cpp                     [EXISTE - ejemplos]
│   ├── test_config_parser.cpp            [CREAR] Tests principales
│   ├── test_bracket_validation.cpp       [CREAR] Tests específicos de brackets
│   └── test_file_helpers.cpp             [CREAR] Tests auxiliares (trim, normalize)
└── data/
    ├── valid/
    │   ├── simple.conf                   [CREAR]
    │   ├── multi_server.conf             [CREAR]
    │   └── nested_locations.conf         [CREAR]
    └── invalid/
        ├── unbalanced_brackets.conf      [CREAR]
        ├── missing_opening.conf          [CREAR]
        ├── missing_closing.conf          [CREAR]
        ├── premature_close.conf          [CREAR]
        └── wrong_extension.txt           [CREAR]
```

---

## Diseño de Test Fixtures

### Fixture Base para ConfigParser

```cpp
// Concepto (no código completo):
// - Constructor que crea archivos temporales en /tmp
// - Destructor que los limpia
// - Métodos helper para crear configs válidos/inválidos rápidamente
// - Paths a archivos de test en tests/data/
```

### Helper Functions

```cpp
// Funciones auxiliares que necesitarás:
// - createTempConfigFile(content) -> returns path
// - readFileContent(path) -> returns string
// - getTestDataPath(filename) -> returns absolute path
```

---

## Casos de Test Prioritarios (Fase 1)

> [!TIP]
> Empieza con estos tests para validar la funcionalidad crítica recién implementada.

### **Test 1: Brackets Balanceados Simples**
```
Archivo: simple_balanced.conf
Contenido:
server {
    listen 80;
}

Validación: ValidateCurlyBrackets() retorna true
```

### **Test 2: Cierre Prematuro** (El bug que corregimos)
```
Archivo: premature_close.conf
Contenido:
}
server {
    listen 80;
}

Validación: ValidateCurlyBrackets() retorna false
```

### **Test 3: Brackets Desbalanceados**
```
Archivo: unbalanced.conf
Contenido:
server {
    listen 80;
    location / {
}

Validación: ValidateCurlyBrackets() retorna false
```

### **Test 4: Parse Completo Válido**
```
Archivo: config/examples/default.conf (ya existe)
Validación: 
- parse() no lanza excepción
- getServerCount() == 1
```

### **Test 5: Extensión Inválida**
```
Archivo: config.txt
Validación: parse() lanza ConfigException con mensaje de extensión
```

---

## Verificación Plan

### Tests Automatizados

**Compilar tests:**
```bash
cd /home/daruuu/CLionProjects/webserv
mkdir -p build && cd build
cmake ..
make unit_tests
```

**Ejecutar todos los tests:**
```bash
./tests/unit/unit_tests
```

**Ejecutar tests específicos por tags:**
```bash
# Solo tests de brackets
./tests/unit/unit_tests [brackets]

# Solo tests de validación de archivos
./tests/unit/unit_tests [file-validation]

# Solo tests de helpers
./tests/unit/unit_tests [helpers]
```

**Verificar cobertura de pruebas:**
- Todos los tests pasan ✅
- Al menos 80% de cobertura en funciones críticas
- Tests de casos edge documentados

---

## Recomendaciones de Implementación

### 🎯 Orden de Implementación Sugerido

1. **Fase 1: Setup Básico**
   - Crear estructura de directorios `tests/data/`
   - Crear archivos de configuración de prueba
   - Configurar CMakeLists.txt si es necesario

2. **Fase 2: Tests de Helpers** (más fáciles)
   - `test_file_helpers.cpp`: TrimLine, NormalizeSpaces, RemoveComments
   - No requieren archivos, solo strings

3. **Fase 3: Tests de Brackets** (prioritario)
   - `test_bracket_validation.cpp`
   - Validar la corrección reciente

4. **Fase 4: Tests de Validación**
   - `test_config_parser.cpp`: ValidateFileExtension, ValidateFilePermissions

5. **Fase 5: Tests de Integración**
   - Parse completo con archivos reales
   - Validación de excepciones

### 📋 Patrón de Test con Catch2

```cpp
// Patrón recomendado:
TEST_CASE("Descripción clara del test", "[tag]") {
    // GIVEN (Arrange)
    ConfigParser parser("path/to/test.conf");
    
    // WHEN (Act)
    bool result = parser.someMethod();
    
    // THEN (Assert)
    REQUIRE(result == expected);
}

// Para tests con excepciones:
TEST_CASE("Parser lanza excepción con extensión inválida", "[exceptions]") {
    ConfigParser parser("invalid.txt");
    
    REQUIRE_THROWS_AS(parser.parse(), ConfigException);
}

// Para tests con secciones:
TEST_CASE("ValidateCurlyBrackets - múltiples casos", "[brackets]") {
    SECTION("Brackets balanceados") {
        // test code
    }
    
    SECTION("Más aperturas que cierres") {
        // test code
    }
    
    SECTION("Cierre prematuro") {
        // test code
    }
}
```

### 🏷️ Sistema de Tags Propuesto

- `[brackets]` - Tests de validación de brackets
- `[file-validation]` - Tests de validación de archivos
- `[helpers]` - Tests de funciones auxiliares
- `[integration]` - Tests de parsing completo
- `[exceptions]` - Tests que validan excepciones
- `[smoke]` - Tests críticos que deben pasar siempre

---

## Consideraciones C++98

> [!WARNING]
> Restricciones importantes para C++98:

- ❌ No usar `std::shared_ptr` / `std::unique_ptr`
- ❌ No usar inicializadores de lista `{}`
- ❌ No usar `auto`
- ❌ No usar lambdas
- ✅ Usar punteros crudos o referencias
- ✅ Usar `std::vector`, `std::string`
- ✅ Catch2 v2.x (single-header) es compatible con C++98

---

## Métricas de Éxito

✅ **Criterios de aceptación:**

1. Al menos **15 test cases** implementados
2. **100% de cobertura** en `ValidateCurlyBrackets()`
3. **80%+ cobertura** en funciones auxiliares
4. Tests para **todos los casos edge** documentados
5. Todos los tests pasan en compilación con `-std=c++98`
6. Documentación clara de cada test case
