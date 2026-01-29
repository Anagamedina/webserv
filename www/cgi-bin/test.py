#!/usr/bin/python3
import os

print("Status: 200 OK")
print("Content-Type: text/plain")
print("")
print("Hello from Python CGI!")
print("Method: " + os.environ.get("REQUEST_METHOD", "Unknown"))
