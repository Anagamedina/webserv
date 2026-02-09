# Ejecutar sin configuracion (Linux)

Este flujo sirve para probar el servidor aunque aun no tengas la parte de
configuracion (ServerBlock/Location). El RequestProcessor usa una respuesta
basica por defecto.

## 1) Compilar
```bash
make
```

## 2) Ejecutar
```bash
./webserver
```

## 3) Probar con curl
En otra terminal:
```bash
curl -v http://127.0.0.1:8080/
```

## Resultado esperado
- Codigo 200 OK
- Body con "OK\n"

Nota: cuando llegue la configuracion real, el flujo sera igual, solo cambiara
la respuesta.
