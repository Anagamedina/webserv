#!/usr/bin/env python3
import sys
import os

print("first line")
print("Content-Type: text/plain")
print("")
print "Hello from debug CGI!"
print(f"Method: {os.environ.get('REQUEST_METHOD', 'Unknown')}")
sys.stderr.write("=== CGI ENDING ===\n")
sys.stderr.flush()
