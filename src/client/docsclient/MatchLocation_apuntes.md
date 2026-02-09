# MatchLocation: para que sirven los strings de path

En `matchLocation()` la idea es **comparar la URI del request con los paths
de cada `LocationConfig`** y elegir la mejor coincidencia.

## 1) Que strings se crean
Cuando exista el getter, cada location da un `path`:
- `"/"` 
- `"/img"`
- `"/cgi"`
- etc

Ese string es **la regla** que define a qué rutas aplica esa location.

## 2) Que se hace con esos strings
Se usan para comparar con la URI del request:

Ejemplo:
- URI: `/img/logo.png`
- Location paths: `/`, `/img`, `/img/icons`

Coincidencias:
- `/` coincide (pero es muy general)
- `/img` coincide (mejor)
- `/img/icons` no coincide

La **mejor** es la que tiene el path mas largo que encaja:
`/img`

## 3) Resultado final
Se devuelve el `LocationConfig*` de la mejor coincidencia.
Ese location luego define:
- root/alias
- metodos permitidos
- autoindex
- CGI, etc

## Resumen corto
Los strings de path **solo sirven para comparar** y elegir la location correcta.

