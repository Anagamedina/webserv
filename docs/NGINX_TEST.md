# Test with Nginx server

Pasos basicos para instalar y ejecutar nginx en local

```bash

cd ~

# descargamos la version que nos interese de nginx la última es la 1.29.5
wget https://nginx.org/download/nginx-1.29.5.tar.gz
tar -xzf nginx-1.29.5.tar.gz
cd nginx-1.29.5
```

### Configure
Configuramos donde queremos instalarlo, en nuestro caso nos va bien en nuestro home,  
en la carpeta nginx, por ejemplo aqui lo instalamos en $HOME/nginx.  
Para instalarlo dentro de nuestro webserver, usar $(pwd)/../nginx.
```bash
./configure --prefix=$HOME/nginx \
    --sbin-path=$HOME/nginx/nginx \
    --conf-path=$HOME/nginx/conf/nginx.conf \
    --pid-path=$HOME/nginx/nginx.pid

# --- para instalarlo dentro del repo webserver:
export WSERV_PATH=$HOME/Documents/42bcn/webserv && ./configure --prefix=$WSERV_PATH/nginx \
    --sbin-path=$WSERV_PATH/nginx \
    --conf-path=$WSERV_PATH/nginx/conf/nginx.conf \
    --pid-path=$WSERV_PATH/nginx/nginx.pid
```

finalmente podemos ejecutar

```bash
make
make install
```

para ejecuntar nginx podemos usar este nginx.conf como base
```nginx
# ~/nginx/conf/nginx.conf
worker_processes  1;

events {
    worker_connections  1024;
}

http {
    include       mime.types;
    default_type  application/octet-stream;

    # Change log paths to local
    error_log  /home/YOUR_USERNAME/nginx/logs/error.log;
    access_log /home/YOUR_USERNAME/nginx/logs/access.log;

    server {
        listen       8080;  # Use 8080+ (non-privileged)
        server_name  localhost;

        location / {
            root   /home/YOUR_USERNAME/nginx/html;
            index  index.html index.htm;
        }
    }
}
```
to execute it:
```bash
./nginx -c <path_to_config_file.conf> -g 'daemon off;' 
```

