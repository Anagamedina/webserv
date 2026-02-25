#!/usr/bin/env python3

import os
import re

# Simple HTML generation showing session demo
html_template = """\
<html>
<head>
    <title>Session Demo</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { border: 1px solid #ccc; padding: 20px; border-radius: 8px; max-width: 500px; }
        h1 { color: #333; }
        .info { background: #f9f9f9; padding: 10px; border-left: 4px solid #0066cc; margin-top: 15px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Welcome to the CGI Session Demo!</h1>
        <p>This page uses Python CGI to read the session cookie set by the WebServer.</p>
        <div class="info">
            <strong>Your Cookie:</strong> {cookie}<br><br>
            <strong>Session ID:</strong> {session_id}
        </div>
        <p><em>Refresh the page to see the visit count increase (tracked by the server's SessionManager).</em></p>
    </div>
</body>
</html>
"""

def main():
    cookie_header = os.environ.get('HTTP_COOKIE', 'No cookie found')
    
    # Extract session_id for display
    session_id = 'Unknown'
    match = re.search(r'session_id=([^;]+)', cookie_header)
    if match:
        session_id = match.group(1)

    print("Content-Type: text/html\r\n\r\n")
    print(html_template.format(cookie=cookie_header, session_id=session_id))

if __name__ == '__main__':
    main()
