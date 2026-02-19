#!/bin/bash

SERVER_BIN="./webserver"
CONFIG="config/default.conf"
PORT=8080

# Kill any running webserv instance to avoid port conflicts
pkill webserv
sleep 1

# Start server
echo "Starting server..."
$SERVER_BIN $CONFIG > /dev/null 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Wait for server to start
sleep 2

# Create a dummy file for testing
mkdir -p www
echo "Hello World" > www/test_head.txt

# Test 1: HEAD request
echo "------------------------------------------------"
echo "Testing HEAD request to /test_head.txt"
curl -v -I http://localhost:$PORT/test_head.txt > head_output.txt 2>&1

# Check headers
echo "Checking headers..."
if grep -q "Content-Length: 12" head_output.txt; then
    echo "PASS: Content-Length is correct (12)"
else
    echo "FAIL: Content-Length missing or incorrect"
    grep "Content-Length" head_output.txt
fi

# Test 2: Verify no body
echo "------------------------------------------------"
echo "Testing raw HEAD request with nc..."
# Send HEAD request
printf "HEAD /test_head.txt HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost $PORT > raw_head_response.txt

# Check if we got headers and NO body.
if grep -q "Hello World" raw_head_response.txt; then
    echo "FAIL: Body received in HEAD request!"
else
    # Verify we actually got a response (headers)
    if grep -q "HTTP/1.1 200 OK" raw_head_response.txt; then
        echo "PASS: No body received in HEAD request."
    else
        echo "FAIL: No valid response received."
        cat raw_head_response.txt
    fi
fi

# Cleanup
rm www/test_head.txt
rm head_output.txt raw_head_response.txt

# Stop server
echo "Stopping server..."
kill $SERVER_PID
