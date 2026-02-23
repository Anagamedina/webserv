#!/usr/bin/env python3

import os

print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>Python CGI Executed</h1>")
print(f"<p>Method: {os.environ.get('REQUEST_METHOD', 'Unknown')}</p>")
print("</body></html>")
