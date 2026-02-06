# 🔍 Code Review: `src/config/`

## 📊 Estado Actual

### Archivos Existentes
- ✅ [ConfigParser.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.hpp) - Parcialmente implementado
- ✅ [ConfigParser.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp) - Parcialmente implementado
- ❌ [ServerConfig.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ServerConfig.hpp) - **VACÍO**
- ❌ [ConfigException.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigException.hpp) - **VACÍO**
- ⚠️ [mainConfig.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/mainConfig.cpp) - Código de prueba

---

## 🚨 Problemas Críticos

### 1. **Método `parse()` declarado como `const`** - INCORRECTO

```cpp
// ❌ ConfigParser.hpp - Línea 16
void parse() const;
```

**Problema**: El método `parse()` debería modificar el estado interno (`servers_`, `rawServerBlocks_`, `serversCount_`), pero está marcado como `const`.

**Solución**:
```cpp
// ✅ Correcto
void parse();  // Sin const
```

---

### 2. **Uso de `std::cout` en lugar de excepciones**

```cpp
// ❌ ConfigParser.cpp - Línea 27
std::cout << "\nError open file :(\n";
```

**Problema**: Los errores deberían lanzar excepciones, no imprimir en consola.

**Solución**:
```cpp
// ✅ Correcto
throw ConfigException("Cannot open config file: " + configFile_);
```

---

### 3. **`ConfigException.hpp` está vacío**

Este archivo es **fundamental** para el manejo de errores pero no está implementado.

---

### 4. **`ServerConfig.hpp` está vacío**

No puedes almacenar configuraciones sin esta clase.

---

### 5. **`mainConfig.cpp` contiene funciones que no pertenecen ahí**

```cpp
// ❌ Funciones en mainConfig.cpp
std::string findFile(...)
std::string getParentDirectory(...)
```

**Problema**: Estas son utilidades generales, no deberían estar en `main`.

**Solución**: Crear `src/config/ConfigUtils.hpp` y `ConfigUtils.cpp`.

---

### 6. **Ruta hardcodeada en `mainConfig.cpp`**

```cpp
// ❌ Línea 84
configPath = "../../config/default.conf";
```

**Problema**: Esta ruta solo funciona desde `cmake-build-debug`. Fallará desde la raíz.

**Solución**:
```cpp
// ✅ Usar ruta relativa desde raíz del proyecto
configPath = "config/default.conf";
```

---

### 7. **Headers innecesarios en `mainConfig.cpp`**

```cpp
// ❌ No se usan
#include <fcntl.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
```

---

### 8. **Falta implementación del parseo real**

El método `parse()` solo valida el archivo pero no lo parsea realmente.

---

## ✅ Aspectos Positivos

1. ✅ Constructor de copia y operador de asignación correctamente implementados en privado
2. ✅ Uso de `explicit` en el constructor
3. ✅ Validación de extensión `.conf`
4. ✅ Uso correcto de `rawServerBlocks_` (preparado para futuro parseo)
5. ✅ Documentación del flujo en `flow-of-parsing`

---

## 🎯 Plan de Mejora

### Fase 1: Implementar Clases Base (CRÍTICO)

#### 1.1 `ConfigException.hpp`
```cpp
#ifndef WEBSERV_CONFIGEXCEPTION_HPP
#define WEBSERV_CONFIGEXCEPTION_HPP

#include <exception>
#include <string>

class ConfigException : public std::exception
{
private:
    std::string message_;
    
public:
    explicit ConfigException(const std::string& msg);
    virtual ~ConfigException() throw();
    virtual const char* what() const throw();
};

#endif
```

#### 1.2 `ServerConfig.hpp`
Debe almacenar:
- Puerto (`listen`)
- Host (`host`)
- Server names (`server_name`)
- Tamaño máximo del body (`client_max_body_size`)
- Páginas de error (`error_page`)
- Locations (`location {}` blocks)

#### 1.3 `LocationConfig.hpp`
Debe almacenar:
- Ruta (`/upload`, `/`, etc.)
- Root directory
- Index files
- Métodos permitidos (GET, POST, DELETE)
- Autoindex on/off
- Upload directory
- Redirecciones

---

### Fase 2: Refactorizar `ConfigParser`

#### 2.1 Cambios en `ConfigParser.hpp`

**Eliminar**:
- `parse() const` → cambiar a `parse()`
- `unsigned int` → usar `size_t`

**Añadir**:
```cpp
private:
    std::string readFileContent();
    void extractServerBlocks(const std::string& content);
    void parseServerBlocks();
    ServerConfig parseServerBlock(const std::string& block);
    LocationConfig parseLocationBlock(const std::string& block);
    
    // Utilidades
    std::string trim(const std::string& str);
    bool isCommentOrEmpty(const std::string& line);
    std::vector<std::string> split(const std::string& str, char delimiter);
```

#### 2.2 Implementar método `parse()` real

```cpp
void ConfigParser::parse()
{
    // 1. Validar archivo
    validateExtensionAndPermissionsFile();
    
    // 2. Leer contenido
    std::string content = readFileContent();
    
    // 3. Extraer bloques server { ... }
    extractServerBlocks(content);
    
    // 4. Parsear cada bloque
    parseServerBlocks();
    
    // 5. Validar configuración final
    validateParsedConfig();
}
```

---

### Fase 3: Organizar Utilidades

Crear `ConfigUtils.hpp` y `.cpp` para:
- `trim()`
- `split()`
- `isCommentOrEmpty()`
- `findMatchingBrace()`

---

### Fase 4: Refactorizar `mainConfig.cpp`

```cpp
int main(int argc, char* argv[])
{
    try
    {
        std::string configPath = (argc == 2) ? argv[1] : "config/default.conf";
        
        ConfigParser parser(configPath);
        parser.parse();
        
        std::vector<ServerConfig> servers = parser.getServers();
        
        // Usar configuración...
        std::cout << "Loaded " << servers.size() << " server(s)" << std::endl;
    }
    catch (const ConfigException& e)
    {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

---

## 📋 Checklist de Mejoras

### Archivos a Crear
- [ ] `ConfigException.hpp` + `.cpp`
- [ ] `ServerConfig.hpp` + `.cpp`
- [ ] `LocationConfig.hpp` + `.cpp`
- [ ] `ConfigUtils.hpp` + `.cpp`

### Archivos a Modificar
- [ ] `ConfigParser.hpp` - Eliminar `const` de `parse()`, añadir métodos privados
- [ ] `ConfigParser.cpp` - Implementar parseo real
- [ ] `mainConfig.cpp` - Limpiar y simplificar

### Archivos a Eliminar/Reemplazar
- [ ] `flow-of-parsing` - Mover info a documentación o comentarios

---

## 🎨 Estructura Final Propuesta

```
src/config/
├── ConfigException.hpp         # Manejo de errores
├── ConfigException.cpp
├── ServerConfig.hpp            # Config de un server { }
├── ServerConfig.cpp
├── LocationConfig.hpp          # Config de un location { }
├── LocationConfig.cpp
├── ConfigParser.hpp            # Parser principal
├── ConfigParser.cpp
├── ConfigUtils.hpp             # Utilidades (trim, split, etc.)
├── ConfigUtils.cpp
└── README.md                   # Documentación del módulo
```

---

## 🔧 Comandos para Testear

```bash
# Compilar
make

# Probar con config por defecto
./webserver

# Probar con config custom
./webserver config/examples/nginx.conf

# Probar con archivo inválido
./webserver invalid.txt  # Debería lanzar excepción
```

---

## 📊 Métricas de Calidad Esperadas

| Métrica | Actual | Objetivo |
|---------|--------|----------|
| Archivos vacíos | 2 | 0 |
| Código duplicado | Alto | Bajo |
| Manejo de errores | `cout` | Excepciones |
| Tests | 0 | 10+ |
| Cobertura | 0% | 80%+ |
| Documentación | Mínima | Completa |

---

## 💡 Próximos Pasos

1. **Implementar clases base** (`ConfigException`, `ServerConfig`, `LocationConfig`)
2. **Refactorizar `ConfigParser`** con parseo real
3. **Crear utilidades** separadas
4. **Escribir tests** unitarios
5. **Documentar** el módulo

¿Por dónde quieres empezar?
