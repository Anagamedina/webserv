#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
print("")
print("OK: echo_env")
print("REQUEST_METHOD=" + os.environ.get("REQUEST_METHOD", ""))
print("QUERY_STRING=" + os.environ.get("QUERY_STRING", ""))
print("CONTENT_LENGTH=" + os.environ.get("CONTENT_LENGTH", ""))
print("SCRIPT_NAME=" + os.environ.get("SCRIPT_NAME", ""))
print("REQUEST_URI=" + os.environ.get("REQUEST_URI", ""))
