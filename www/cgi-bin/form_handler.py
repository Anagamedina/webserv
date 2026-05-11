#!/usr/bin/env python3
"""
Python CGI script to test POST data handling
Handles both GET query strings and POST form data
"""
import os
import sys
import urllib.parse

# Read POST data from stdin if present
content_length = os.environ.get('CONTENT_LENGTH', '0')
post_data = ''
if content_length and content_length != '0':
    try:
        post_data = sys.stdin.read(int(content_length))
    except:
        post_data = ''

# Get query string from environment
query_string = os.environ.get('QUERY_STRING', '')

# Parse data
get_params = urllib.parse.parse_qs(query_string) if query_string else {}
post_params = urllib.parse.parse_qs(post_data) if post_data else {}

# Output HTTP headers
print("Content-Type: text/html")
print("Status: 200 OK")
print()

# Output HTML response
print("<!DOCTYPE html>")
print("<html><head><base href=\"/static/\"><title>Python Form Handler</title></head>")
print("<body>")
print("<h1>Python CGI - Form Data Handler</h1>")
print(f"<p><strong>Request Method:</strong> {os.environ.get('REQUEST_METHOD', 'N/A')}</p>")
print(f"<p><strong>Content Length:</strong> {content_length}</p>")
print("<hr>")

print("<h2>GET Parameters (Query String):</h2>")
if get_params:
    print("<ul>")
    for key, values in get_params.items():
        for value in values:
            print(f"<li><strong>{key}:</strong> {value}</li>")
    print("</ul>")
else:
    print("<p><em>No GET parameters</em></p>")

print("<h2>POST Parameters (Body):</h2>")
if post_params:
    print("<ul>")
    for key, values in post_params.items():
        for value in values:
            print(f"<li><strong>{key}:</strong> {value}</li>")
    print("</ul>")
else:
    print("<p><em>No POST parameters</em></p>")

print("<hr>")
print("<h2>Raw POST Data:</h2>")
if post_data:
    print(f"<pre>{post_data}</pre>")
else:
    print("<p><em>No POST data</em></p>")

print("<hr>")
print('<h2>Test Form</h2>')
print('<form method="POST" action="cgi-bin/form_handler.py">')
print('  <label>Name: <input type="text" name="name" value="John"></label><br>')
print('  <label>Email: <input type="email" name="email" value="john@example.com"></label><br>')
print('  <label>Message: <textarea name="message">Hello from CGI!</textarea></label><br>')
print('  <button type="submit">Submit via POST</button>')
print('</form>')

print("</body></html>")
