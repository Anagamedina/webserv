# Testing POST and GET

## Compile
```bash
make clean && make
```

## Start server
```bash
 ./webserver config/test_multi_server.conf
```
## Test GET

curl -v http://localhost:8080/index.html

## Test POST

### Create file if needed
dd if=/dev/zero of=1mb.test bs=1M count=1
dd if=/dev/zero of=6mb.test bs=1M count=6


### Upload a file
curl -X POST --data-binary @file.txt http://localhost:8080/upload
curl -v -X POST --data-binary @1mb.test http://127.0.0.1:8080/upload/testfile.bin
curl -v -X POST --data-binary @6mb.test http://127.0.0.1:8081/youpi/bigfile.bin
### Upload with custom filename
curl -X POST --data-binary "test content" http://localhost:8080/upload/myfile.txt

## Verify
ls -la ./www/uploads

check that the new file has been created
