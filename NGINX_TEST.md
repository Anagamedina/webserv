# Test with Nginx server

download the tar ball from oficial website

compile with
```bash
./configure \
  --prefix=$(pwd)/build \
  --with-http_ssl_module # in case you wanna use ssl (no needed for webserv)

make
```
exec is inside build/sbin/nginx

to pass a .conf file use the flag -c


