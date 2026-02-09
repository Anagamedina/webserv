 # Integracion RequestProcessor + CGI (apuntes)

Este documento resume como se enlaza la parte de RequestProcessor con CGI y
que informacion debe intercambiarse con Carles para integrarlo bien.

## 1) Dónde se conecta CGI
- El RequestProcessor decide si una peticion es CGI o estatica.
- Si es CGI, delega la ejecucion a la capa de CGI (Carles).

## 2) Qué debe exponer CGI (Carles)
Carles debe dar una interfaz clara, por ejemplo:

```
HttpResponse CgiHandler::execute(const HttpRequest& request,
                                 const std::string& scriptPath,
                                 const ServerBlock& server,
                                 const Location& location);
```

O si no devuelve HttpResponse, al menos:
- la salida (headers + body) en un buffer
- el status code resultante
- errores (si el script falla)

## 3) Qué necesita RequestProcessor (tú)
Para decidir CGI necesitas:
- Configuracion (ServerBlock / Location)
- Saber si la location es CGI (extension o flag)
- Saber la ruta real del script (root/alias + URI)
- Metodo permitido (GET/POST normalmente)

## 4) Flujo completo (resumen)
1. Matching (Host + location)
2. Validacion (metodo, body size, redireccion)
3. Resolver path real
4. Si es CGI:
   - llamar a CgiHandler con request + config
   - convertir salida en HttpResponse
5. Si NO es CGI:
   - servir archivo estatico

## 5) Qué debes pedirle a Carles
- La firma exacta de la funcion CGI (nombre, parametros, retorno)
- Formato de la salida (headers + body separados o ya procesados)
- Manejo de errores (como indica un fallo)

## 6) Qué debes darle tú a Carles
- HttpRequest completo (metodo, path, headers, body)
- Ruta real del script (path resuelto)
- Configuracion activa (server y location)

Con esto el processor queda unificado con CGI.

