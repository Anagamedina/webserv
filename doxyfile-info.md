Ver la documentación de Doxygen "en la web":

Localmente (Visualización rápida): Puedes abrir el archivo directamente en tu navegador. En tu terminal, puedes ejecutar (dependiendo de tu sistema):

``` bash
# En Linux
xdg-open docs/html/index.html
```

O simplemente buscar el archivo en tu explorador de archivos y abrirlo con Chrome/Firefox.

Servidor local (Python): Si quieres verlo a través de un servidor (como si estuviera en la web), puedes usar Python desde la carpeta de documentación:
``` bash
cd docs/html && python3 -m http.server 8080
Luego abre en tu navegador: http://localhost:8080
```

GitHub Pages (En la web real): Si tu proyecto está en GitHub, puedes ir a Settings > Pages y seleccionar la carpeta docs/html (o docs) de la rama main para que se publique automáticamente en una URL tipo https://usuario.github.io/webserv/.
