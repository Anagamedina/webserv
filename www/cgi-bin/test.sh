#!/bin/bash

echo -e "Content-Type: text/html\r\n\r"
echo "<html><body>"
echo "<h1>Bash CGI Executed</h1>"
echo "<p>Method: ${REQUEST_METHOD}</p>"
echo "</body></html>"
