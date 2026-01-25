# Testing manual (sin framework)

Este proyecto no usa framework de tests todavía. Puedes empezar con **tests manuales** simples usando `main()` y mensajes por consola.

## Objetivo

- Validar lógica básica en clases HTTP (por ejemplo `HttpRequest`) sin levantar el servidor.
- Empezar con pruebas simples y ampliar después cuando `HttpParser` esté implementado.

## Cómo compilar un test manual

Ejemplo para compilar un test básico de `HttpRequest`:

```
g++ -std=c++98 -I./src -o tests/manual_http_request \
    tests/manual_http_request.cpp src/http/HttpRequest.cpp
```

Luego ejecuta:

```
./tests/manual_http_request
```

## Siguiente paso cuando `HttpParser` esté listo

Crear un test similar para `HttpParser` que:
- Envíe texto HTTP por partes (simulando datos parciales del socket).
- Verifique que el parser cambia de estado.
- Revise que el `HttpRequest` final contiene método, path, headers y body correctos.

