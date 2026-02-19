#!/bin/bash
cd /home/carles/Documents/42bcn/webserv

pkill -9 webserver || true
sleep 2

# Start server
./webserver configs/default.conf > /tmp/final_server.log 2>&1 &
SERVER_PID=$!

# Wait for startup
sleep 2

# Test
timeout 5 curl -s http://127.0.0.1:8080/cgi-bin/test.py > /tmp/final_response.txt 2>&1
CURL_EXIT=$?

# Kill server
sleep 1
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

# Show results
echo "========== Curl Response =========="
cat /tmp/final_response.txt
echo ""
echo "========== Curl Exit Code: $CURL_EXIT =========="
echo ""
echo "========== Server Log (last 30 lines) =========="
tail -30 /tmp/final_server.log
