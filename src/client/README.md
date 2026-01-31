# Cliente y RequestProcessor (resumen)

Este README resume la parte de cliente, processor y cómo encajan con el server.

## Client (socket + estado)
Responsabilidad:
- Recibir bytes del socket (handleRead).
- Pasarlos al HttpParser.
- Cuando hay request completa, pedir respuesta al RequestProcessor.
- Enviar la respuesta serializada (handleWrite).

Detalles importantes:
- No usar errno tras read/write (epoll decide cuándo leer/escribir).
- En LT, una llamada a recv()/send() por evento.
- needsWrite() indica si hay datos pendientes en _outBuffer.
- lastActivity se actualiza en lecturas/escrituras.
- STATE_CLOSED indica que el server debe cerrar el fd.

Firma actual:
```
Client(int fd, const std::vector<ServerBlock>* configs);
```

## RequestProcessor (cerebro)
Responsabilidad:
- Recibe HttpRequest + configs + parseError.
- Decide la respuesta (por ahora básica).
- Rellena un HttpResponse.

Firma actual:
```
void process(const HttpRequest& request,
             const std::vector<ServerBlock>* configs,
             bool parseError,
             HttpResponse& response);
```

Pendientes (TODO):
- Resolver root + path y leer archivos reales.
- Integrar CGI (delegar a CgiHandler de Carles).

## Tests manuales
Se añadieron dos tests de consola:
- `tests/manual_processor/manual_request_processor.cpp`
- `tests/manual_client/manual_client.cpp`

Con Makefile:
```
make test_request_processor
make test_client
```

Con CMake (Linux):
```
mkdir build && cd build
cmake ..
make manual_request_processor manual_client
```

## Diagrama (SVG local)

![Flujo de RequestProcessor](docsclient/mermaid%20de%20process.svg)


