# Flujo CGI y fds (quien los maneja)

## Idea central
El `RequestProcessor` **no maneja fds**.  
El que maneja fds y epoll es el **ServerManager**, y el que inicia el CGI es el **Client**.

---

## Flujo paso a paso (actual + ideal)

1) **ServerManager** acepta conexión → crea `Client(fd)`
2) **Client** lee bytes → parser → crea `HttpRequest`
3) **RequestProcessor** decide: CGI o estático
4) Si es **CGI**:
   - El **Client** llama a `CgiExecutor`
   - `CgiExecutor` crea pipes (`pipe_in`, `pipe_out`) y `fork`
   - Devuelve un `CgiProcess` con esos fds
5) **Client** pide a `ServerManager` registrar `pipe_in` (EPOLLOUT) y `pipe_out` (EPOLLIN)
6) **ServerManager** recibe eventos del pipe y los reenvía al **Client**
7) **Client** escribe el body al CGI y lee la salida
8) **Client** construye `HttpResponse` → envía al socket

---

## Por qué no lo hace RequestProcessor
Porque `RequestProcessor` solo **decide lógica** (qué responder),  
no tiene el socket ni conoce epoll.

---

## Resumen corto
- **RequestProcessor**: decide CGI o estático.
- **Client**: crea CGI y pasa fds al ServerManager.
- **ServerManager**: registra fds en epoll y entrega eventos.
