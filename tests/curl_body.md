Send body
```sh
curl -i -X POST "http://127.0.0.1:8080/cgi-bin/echo_body.py" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=carles&msg=hola"
```
max body size
```sh
curl -i -X POST -H "Content-Type: text/plain" --data "BODY IS HERE and this is definitely longer than twenty bytes" http://localhost:8080
```
method not allowed
```sh
curl -i -X POST -H "Content-Type: text/plain" --data "BODY IS HERE and this is definitely longer than twenty bytes" http://localhost:8081/docs
```
