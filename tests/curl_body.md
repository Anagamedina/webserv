Send body
```sh
curl -i -X POST "http://127.0.0.1:8080/cgi-bin/echo_body.py" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=carles&msg=hola"
```
