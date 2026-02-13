#!/bin/bash
cd /home/carles/Documents/42bcn/webserv

# Kill existing server
pkill -9 webserver 2>/dev/null || true
sleep 1

# Start server with logging
./webserver configs/default.conf > /tmp/server_debug.log 2>&1 &
SERVER_PID=$!
sleep 3

# Make request
curl -s http://127.0.0.1:8080/cgi-bin/test.py > /tmp/curl_response.txt 2>&1

# Give it time to finish
sleep 1

# Kill server
kill -9 $SERVER_PID 2>/dev/null || true

# Show results
echo "===================="
echo "Server Log (filtered)"
echo "===================="
grep -E "File:|CGI|Processing|connected" /tmp/server_debug.log || echo "No matches found"

echo ""
echo "===================="
echo "Curl Response"
echo "===================="
cat /tmp/curl_response.txt || echo "Empty"
