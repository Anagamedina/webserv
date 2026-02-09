# Timeout en el servidor (quien lo maneja)

## Idea simple
El **timeout** no lo "hace" el `Client`.  
El que decide cerrar por inactividad es el **ServerManager**.

---

## Qué hace cada parte

### 1) Client
- Cuando lee o escribe, actualiza `_lastActivity`.
- Solo guarda la "última vez que hubo actividad".
- No conoce a los otros clientes.
- No debe cerrar sockets por timeout global.

### 2) ServerManager
- En su loop llama a `checkTimeouts()`.
- Compara `now` con `client->getLastActivity()`.
- Si pasaron X segundos → cierra el cliente.
- Tiene la lista completa de clientes.

---

## Por qué es así
El `Client` no puede cerrar a otros ni decidir tiempos globales.  
El `ServerManager` sí tiene la lista completa de clientes y el control del loop.

---

## Qué significa "actividad"
- **Lectura**: cuando llega data del cliente.
- **Escritura**: cuando enviamos bytes de respuesta.
- Si no hay ninguna de esas dos cosas durante X segundos, consideramos la conexión "muerta".

---

## Qué problema evita el timeout
- Clientes que se conectan y nunca envían nada.
- Clientes que dejan la conexión abierta sin cerrar (socket colgado).
- Acumulación de fds abiertos que bloquean el servidor.

---

## Ejemplo mental (linea de tiempo)
Supongamos `TIMEOUT = 60s`.

1) t=0: cliente conecta → `_lastActivity = 0`
2) t=10: cliente envía request → `_lastActivity = 10`
3) t=15: servidor responde → `_lastActivity = 15`
4) t=80: no hubo más actividad  
   → `80 - 15 = 65 > 60` → timeout → se cierra.

---

## Flujo resumido
1) `Client` lee/escribe → `_lastActivity = time(0)`
2) `ServerManager::checkTimeouts()` revisa cada cliente
3) Si `now - lastActivity > TIMEOUT` → `handleClientDisconnect()`

---

## Dónde está ahora
- `_lastActivity` se actualiza en `Client::handleRead()` y `Client::handleWrite()`
- El cierre por timeout está en `ServerManager::checkTimeouts()`

---

## Nota de implementación
El timeout **no necesita epoll**, solo comparar tiempos.
Se puede hacer en cada loop o cada N segundos.
## Flujo resumido
1) `Client` lee/escribe → `_lastActivity = time(0)`
2) `ServerManager::checkTimeouts()` revisa cada cliente
3) Si `now - lastActivity > TIMEOUT` → `handleClientDisconnect()`

---

## Dónde está ahora
- `_lastActivity` se actualiza en `Client::handleRead()` y `Client::handleWrite()`
- El cierre por timeout está en `ServerManager::checkTimeouts()`
