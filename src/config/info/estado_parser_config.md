# 📊 Estado Actual del Módulo de Parseo de Configuración

**Fecha de análisis:** 2026-02-04  
**Progreso estimado:** 90-95% ✅

---

## ✅ IMPLEMENTADO (Lo que YA tienes)

### 1. Arquitectura y Estructura
- [x] **ConfigParser** (582 líneas) - Parser principal
- [x] **ServerConfig** (130 líneas) - Configuración de servidor
- [x] **LocationConfig** (124 líneas) - Configuración de location
- [x] **ConfigUtils** (211 líneas) - Utilidades y helpers
- [x] **ConfigException** - Manejo de errores
- [x] **namespaces.hpp** (98 líneas) - Constantes organizadas

### 2. Validaciones Básicas
- [x] Extensión `.conf` válida
- [x] Permisos de archivo (lectura)
- [x] Llaves balanceadas `{}`
- [x] Limpieza de comentarios (`#`)
- [x] Normalización de espacios
- [x] Exportación a logs para debugging

### 3. Directivas de Servidor (Server Block)
- [x] `listen` - Puerto e IP
  - [x] `listen 8080;` (solo puerto)
  - [x] `listen 127.0.0.1:8080;` (IP:puerto)
  - [x] `listen localhost;` (solo host)
- [x] `server_name` - Nombre del servidor
- [x] `root` - Directorio raíz
- [x] `index` - Archivo índice (⚠️ solo uno, ver mejoras)
- [x] `client_max_body_size` - Tamaño máximo del body
- [x] `error_page` - Páginas de error personalizadas
  - [x] Múltiples códigos: `error_page 500 502 503 /error.html;`

### 4. Directivas de Location (Location Block)
- [x] `root` - Directorio raíz específico
- [x] `index` - Archivos índice (múltiples)
- [x] `autoindex` - Directory listing (on/off)
- [x] `methods` / `allow_methods` / `limit_except` - Métodos HTTP permitidos
- [x] **`return`** - Redirects HTTP ✅ **MEJORADO**
  - [x] `return 301 /new-url;` (código + URL)
  - [x] `return http://example.com;` (solo URL, asume 302)
  - [x] Validación de código 3xx (300-399)
  - [x] Manejo de errores para argumentos inválidos
- [x] **`upload_store`** - Directorio para uploads ✅ **MEJORADO**
  - [x] Validación de número de argumentos
  - [x] Validación de path vacío
  - [x] Validación de caracteres peligrosos (`\0`, `\n`, `\r`)

### 5. Location Modifiers ✅ **NUEVO**
- [x] `location = /path` - Exact match
- [x] `location ^~ /path` - Preferential prefix
- [x] Parseo correcto de modifiers

### 6. Utilidades (ConfigUtils)
- [x] `trimLine()` - Eliminar whitespace
- [x] `removeComments()` - Eliminar comentarios
- [x] `normalizeSpaces()` - Normalizar espacios
- [x] `split()` - Tokenización
- [x] `removeSemicolon()` - Limpiar punto y coma
- [x] `stringToInt()` - Conversión con validación
- [x] `fileExists()` - Verificar existencia de archivo
- [x] **`isValidPath()`** - Validar rutas ✅ **NUEVO**
- [x] `exportContentToLogFile()` - Exportar logs

### 7. Validaciones Avanzadas
- [x] **Redirect code** - Validación en `setRedirectCode()` (300-399)
- [x] **Upload path** - Validación de caracteres peligrosos
- [x] **String to int** - Validación de overflow y caracteres inválidos

### 8. Gestión de Errores
- [x] Mensajes de error descriptivos en `namespaces.hpp`
- [x] Excepciones específicas para cada tipo de error
- [x] Validación de argumentos en directivas

---

## ❌ FALTA IMPLEMENTAR (Pendiente)

### 1. Soporte para CGI 🔴 **CRÍTICO**
**Prioridad:** ALTA (Requerido por subject)

El subject requiere parsear configuración CGI. Necesitas agregar:

```nginx
location ~ \.php$ {
    cgi_pass /usr/bin/php-cgi;
    # O alternativamente:
    fastcgi_pass 127.0.0.1:9000;
}
```

**Archivos a modificar:**
- [ ] `LocationConfig.hpp` - Agregar `cgi_pass_` o `cgi_extension_`
- [ ] `LocationConfig.cpp` - Implementar getters/setters
- [ ] `ConfigParser.cpp` - Parsear directiva CGI
- [ ] `namespaces.hpp` - Agregar constante `cgi_pass`

**Código sugerido:**
```cpp
// En LocationConfig.hpp
private:
    std::string cgi_pass_;  // /usr/bin/php-cgi

// En ConfigParser.cpp (dentro del loop de location)
else if (locTokens[0] == "cgi_pass")
{
    if (locTokens.size() != 2) {
        throw ConfigException("Invalid arguments in 'cgi_pass'");
    }
    loc.setCgiPass(config::utils::removeSemicolon(locTokens[1]));
}
```

---

### 2. Validaciones Faltantes 🟡 **IMPORTANTE**

#### 2.1 Validación de Puerto
**Ubicación:** `ServerConfig::setPort()`

```cpp
void ServerConfig::setPort(int port)
{
    if (port < 1 || port > 65535) {  // ❌ FALTA
        throw ConfigException("Invalid port: must be 1-65535");
    }
    listen_port_ = port;
}
```

#### 2.2 Validación de Códigos HTTP
**Ubicación:** `ServerConfig::addErrorPage()`

```cpp
void ServerConfig::addErrorPage(int code, const std::string& path)
{
    if (code < 100 || code > 599) {  // ❌ FALTA
        throw ConfigException("Invalid HTTP status code");
    }
    error_pages_.insert(std::make_pair(code, path));
}
```

#### 2.3 Validación de `autoindex`
**Ubicación:** `ConfigParser.cpp` línea 467-472

Actualmente acepta cualquier valor. Debería validar:
```cpp
if (val != "on" && val != "off") {  // ❌ FALTA
    throw ConfigException("autoindex must be 'on' or 'off'");
}
```

---

### 3. Mejoras de Consistencia 🟡 **MEDIA**

#### 3.1 `index` en ServerConfig
**Problema:** En `ServerConfig` solo guardas un `index`, pero en `LocationConfig` usas vector.

```cpp
// ServerConfig.hpp - ACTUAL
std::string index_;  // ❌ Solo uno

// DEBERÍA SER:
std::vector<std::string> index_;  // ✅ Múltiples
```

**Nginx permite:**
```nginx
server {
    index index.html index.htm default.html;  # Múltiples
}
```

---

### 4. Validación de Orden de Llaves 🟢 **BAJA**
**Ubicación:** `ConfigParser.cpp` línea 85

```cpp
// TODO: need to fix error order of brackets: '} {' should be error but now is
```

**Problema:** No detecta orden incorrecto como `} {`.

**Solución sugerida:**
```cpp
bool ConfigParser::ValidateCurlyBrackets() const
{
    int countBrackets = 0;
    char prevChar = '\0';
    
    for (size_t i = 0; i < clean_file_str_.size(); ++i)
    {
        char c = clean_file_str_[i];
        
        // Detectar } {
        if (prevChar == '}' && c == '{') {
            return false;  // Orden inválido
        }
        
        if (c == '{') {
            ++countBrackets;
        }
        else if (c == '}') {
            --countBrackets;
            if (countBrackets < 0) {
                return false;
            }
        }
        
        if (!std::isspace(c)) {
            prevChar = c;
        }
    }
    return countBrackets == 0;
}
```

---

### 5. Testing 🟡 **IMPORTANTE**

**Actualmente:** Solo 1 test unitario

**Tests que faltan:**
- [ ] Test de extensión inválida
- [ ] Test de archivo inexistente
- [ ] Test de parseo de `listen`
- [ ] Test de múltiples servers
- [ ] Test de location blocks
- [ ] Test de `error_page` múltiple
- [ ] Test de `return` con validaciones
- [ ] Test de `upload_store` con validaciones
- [ ] Test de location modifiers

---

### 6. Limpieza de Código 🟢 **BAJA**

#### 6.1 Código comentado
Hay código comentado que debería eliminarse:
- Línea 35, 45 en `ConfigParser.cpp`
- Línea 564-571 en `ConfigParser.cpp` (directorio existe)

#### 6.2 Include duplicado
**Ubicación:** `ConfigParser.cpp` líneas 8-9
```cpp
#include <string>
#include <string>  // ❌ Duplicado
```

---

## 🐛 BUGS ENCONTRADOS

### Bug 1: Lógica invertida en `isValidPath()` ❌ **CRÍTICO**
**Ubicación:** `ConfigParser.cpp` línea 558

```cpp
if (config::utils::isValidPath(uploadPathClean))  // ❌ LÓGICA INVERTIDA
{
    throw ConfigException(...);  // Lanza error si es VÁLIDO
}
```

**Problema:** `isValidPath()` retorna `true` si el path es válido, pero tú lanzas excepción cuando es `true`.

**Solución:**
```cpp
if (!config::utils::isValidPath(uploadPathClean))  // ✅ Agregar !
{
    throw ConfigException(...);
}
```

---

## 📋 Checklist de Completitud

### Funcionalidades Core
| Funcionalidad | Estado |
|---------------|--------|
| Validar extensión `.conf` | ✅ |
| Validar permisos de archivo | ✅ |
| Limpiar comentarios | ✅ |
| Validar llaves balanceadas | ✅ |
| Validar orden de llaves | ❌ |
| Extraer bloques `server` | ✅ |
| Parsear directivas de server | ✅ |
| Parsear bloques `location` | ✅ |
| Parsear directivas de location | ✅ |

### Directivas Server
| Directiva | Estado |
|-----------|--------|
| `listen` (IP:PORT, PORT, IP) | ✅ |
| `server_name` | ✅ |
| `root` | ✅ |
| `index` (múltiples) | ⚠️ Solo uno |
| `client_max_body_size` | ✅ |
| `error_page` (múltiples códigos) | ✅ |

### Directivas Location
| Directiva | Estado |
|-----------|--------|
| `root` | ✅ |
| `index` (múltiples) | ✅ |
| `autoindex` | ✅ |
| `methods` / `allow_methods` / `limit_except` | ✅ |
| `return` (redirects) | ✅ |
| `upload_store` | ✅ |
| **`cgi_pass`** | ❌ **FALTA** |
| Location modifiers (`=`, `^~`) | ✅ |

### Validaciones
| Validación | Estado |
|------------|--------|
| Extensión de archivo | ✅ |
| Permisos de archivo | ✅ |
| Llaves balanceadas | ✅ |
| Orden de llaves | ❌ |
| Rango de puerto (1-65535) | ❌ |
| Códigos HTTP válidos (100-599) | ❌ |
| Valores de `autoindex` (on/off) | ❌ |
| Redirect code (300-399) | ✅ |
| Upload path válido | ✅ (pero bug en línea 558) |

### Testing
| Test | Estado |
|------|--------|
| Test de llaves inválidas | ✅ |
| Test de extensión inválida | ❌ |
| Test de archivo inexistente | ❌ |
| Test de parseo de `listen` | ❌ |
| Test de múltiples servers | ❌ |
| Test de location blocks | ❌ |
| Test de `error_page` múltiple | ❌ |
| Test de `return` | ❌ |
| Test de `upload_store` | ❌ |

---

## 🎯 Prioridades de Implementación

### 🔴 CRÍTICO (Hacer AHORA)
1. **Arreglar bug en línea 558** - Lógica invertida en `isValidPath()`
2. **Agregar soporte para CGI** - Requerido por subject
3. **Validar puerto** (1-65535)
4. **Validar códigos HTTP** (100-599)

### 🟡 IMPORTANTE (Hacer pronto)
5. **Validar `autoindex`** (on/off)
6. **Cambiar `index` en ServerConfig a vector**
7. **Agregar más tests unitarios** (al menos 5-6)

### 🟢 OPCIONAL (Si tienes tiempo)
8. **Mejorar validación de llaves** (detectar `} {`)
9. **Limpiar código comentado**
10. **Eliminar include duplicado**

---

## 📊 Estadísticas del Código

| Archivo | Líneas | Estado |
|---------|--------|--------|
| ConfigParser.cpp | 582 | 95% completo |
| ServerConfig.cpp | 130 | 90% completo |
| LocationConfig.cpp | 124 | 95% completo |
| ConfigUtils.cpp | 211 | 100% completo |
| namespaces.hpp | 98 | 100% completo |
| **TOTAL** | **1145** | **~93%** |

---

## ⏱️ Tiempo Estimado para Completar

| Tarea | Tiempo |
|-------|--------|
| Arreglar bug línea 558 | 5 min |
| Agregar soporte CGI | 1-2 horas |
| Validaciones (puerto, HTTP, autoindex) | 30-45 min |
| Cambiar `index` a vector | 30 min |
| Tests unitarios (5-6 tests) | 2-3 horas |
| Limpieza de código | 15 min |
| **TOTAL MÍNIMO** | **2-3 horas** |
| **TOTAL COMPLETO** | **4-6 horas** |

---

## ✅ Conclusión

**Tu módulo de parseo está en excelente estado (90-95% completo).**

### Fortalezas:
- ✅ Arquitectura sólida y bien organizada
- ✅ Validaciones robustas implementadas
- ✅ Soporte para la mayoría de directivas
- ✅ Manejo de errores completo
- ✅ Código C++98 compliant

### Crítico para completar:
1. 🔴 **Arreglar bug línea 558** (5 minutos)
2. 🔴 **Agregar CGI** (requerido por subject)
3. 🟡 **Validaciones de valores**
4. 🟡 **Tests unitarios**

**¡Estás muy cerca de terminar!** 🎉
