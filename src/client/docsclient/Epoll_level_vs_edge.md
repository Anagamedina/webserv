# Apuntes: Level Triggered vs Edge Triggered (epoll)

Estos apuntes explican que es Level Triggered (LT) y Edge Triggered (ET) y
por que hemos elegido Level Triggered en nuestro servidor.

## 1) Idea general
Epoll puede avisar de dos formas cuando un socket esta listo:

- Level Triggered (LT): notifica mientras el socket siga listo.
- Edge Triggered (ET): notifica solo cuando cambia el estado.

## 2) Level Triggered (LT)
Que significa:
- Si hay datos disponibles para leer, epoll avisa una y otra vez.
- Si el socket sigue escribible, epoll sigue avisando.

Consecuencia:
- Puedes leer o escribir una sola vez por evento.
- Si no terminas, epoll te volvera a despertar.

Ventajas:
- Mas simple y seguro.
- Menos riesgo de perder datos si no lees todo.
- Mejor para principiantes o cuando el flujo es complejo.

Desventajas:
- Puede generar mas eventos (mas CPU) si no gestionas EPOLLOUT bien.

## 3) Edge Triggered (ET)
Que significa:
- Epoll avisa solo cuando el estado cambia de "no listo" a "listo".
- Si no consumes todo, no vuelve a avisar.

Consecuencia:
- Debes leer o escribir TODO hasta EAGAIN.
- Si no drenas el socket, puedes quedarte bloqueado para siempre.

Ventajas:
- Menos eventos, mas eficiente si se implementa bien.

Desventajas:
- Mas dificil de implementar.
- Facil cometer errores y perder datos.

## 4) Por que usamos Level Triggered
En nuestro proyecto elegimos LT por:

- Seguridad: si no lees todo, epoll te vuelve a avisar.
- Simplicidad: la logica de Client es mas simple.
- Estabilidad: menos riesgo de bugs con el parser y el flujo de respuesta.

## 5) Regla importante con LT
Como usamos LT, el server debe activar EPOLLOUT solo cuando hay datos pendientes.
Si no, epoll puede avisar constantemente y consumir CPU.

En resumen:
- LT = mas facil y seguro
- ET = mas rapido pero mas dificil

