# Guía de Implementación del Parser de Configuración (.conf)

Esta guía detalla la estrategia recomendada para extraer y procesar la información de tu archivo de configuración `webserv.conf` utilizando las clases `ServerConfig` y `LocationConfig` que ya hemos preparado.

## 1. Estrategia: Máquina de Estados (State Machine)

La mejor forma de parsear un archivo anidado (donde tienes bloques dentro de bloques, como `server { location { ... } }`) es usar una **Máquina de Estados Simple**.

### ¿Por qué?
Porque el significado de una línea depende del contexto.
*   `root /var/www;` dentro de `server` es la raíz global del servidor.
*   `root /html;` dentro de `location /` solo aplica a esa ruta.

### Estados Necesarios
Definiremos un enum para saber dónde estamos parado mientras leemos línea por línea:
1.  **`OUTSIDE`**: Esperando encontrar `server {`.
2.  **`IN_SERVER`**: Dentro de un bloque server, leyendo puertos, hosts, o esperando `location`.
3.  **`IN_LOCATION`**: Dentro de un bloque location, leyendo métodos, autoindex, etc.

---

## 2. Flujo de Parseo (Paso a Paso)

El flujo debe ocurrir en `ConfigParser.cpp`. Actualmente tienes el código comentado, pero esta es la lógica que debes implementar:

### Paso A: Limpieza Previa (Ya implementado)
El archivo se lee completo, se quitan comentarios (`#`) y espacios extra. Tienes un string gigante o un vector de strings limpios.

### Paso B: El Bucle Principal
Iteras sobre el contenido limpio (preferiblemente línea por línea o token por token).

```cpp
ServerConfig currentServer;
LocationConfig currentLocation;
ParserState state = OUTSIDE;

for (cada linea en el archivo) {
    // 1. Dividir la línea en palabras (tokens)
    // Ej: "listen 80;" -> ["listen", "80;"]
    tokens = split(linea);
    
    // 2. Máquina de Estados
    switch (state) {
        
        case OUTSIDE:
            if (token[0] == "server" && token[1] == "{")
                state = IN_SERVER;
                currentServer = ServerConfig(); // Reiniciar config
            break;

        case IN_SERVER:
            if (token[0] == "listen") 
                currentServer.setPort(parsePort(token[1]));
            else if (token[0] == "location") {
                state = IN_LOCATION;
                currentLocation = LocationConfig(); // Reiniciar location
                currentLocation.setPath(token[1]);
            }
            else if (token[0] == "}") {
                // Fin del servidor
                servers_.push_back(currentServer); // Guardar servidor completado
                state = OUTSIDE;
            }
            break;

        case IN_LOCATION:
            if (token[0] == "root")
                currentLocation.setRoot(token[1]);
            else if (token[0] == "methods")
                // Loop para añadir todos los métodos
            else if (token[0] == "}") {
                // Fin de la location
                currentServer.addLocation(currentLocation); // Guardar location en el servidor
                state = IN_SERVER; // Volvemos al contexto del servidor
            }
            break;
    }
}
```

---

## 3. Detalles de Implementación Clave

### 3.1 Manejo del Punto y Coma (`;`)
Las directivas en Nginx terminan en `;`.
*   **Problema**: `listen 80;` -> El token será `"80;"`.
*   **Solución**: Debes crear una función `cleanToken()` que elimine el `;` final antes de convertir a número o guardar el string.

### 3.2 Directivas Múltiples (Vectores)
Para `index index.html index.htm;`:
*   Detectas `index`.
*   Haces un bucle desde `i = 1` hasta el final de la línea.
*   Cada elemento se añade con `currentLocation.addIndex()`.

### 3.3 Validaciones
*   Si encuentras un token desconocido (ej. `banana 123;`), debes lanzar una `ConfigException`.
*   Si intentas cerrar con `}` y estás en `OUTSIDE`, es un error de sintaxis.

---

## 4. Siguientes Pasos Recomendados

1.  **Descomentar** la función `parseServerBlock` en `ConfigParser.cpp`.
2.  **Implementar la tokenización**: Usar `std::stringstream` para partir la línea.
3.  **Implementar el `switch` o `if/else`** anidado siguiendo la lógica de arriba.
4.  **Probar** con el archivo `default.conf`.
