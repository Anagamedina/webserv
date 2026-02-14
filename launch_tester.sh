#!/bin/bash

# ---------------------------
# Configuration
# ---------------------------
SERVER_EXEC=./webserv
CONFIG_FILE=config/tester.conf
TESTER_EXEC=testers/tester

# Default URL if none provided
URL=${1:-http://localhost:8080}

# ---------------------------
# Launch server
# ---------------------------
#echo "[*] Starting webserv with config: $CONFIG_FILE"
#$SERVER_EXEC $CONFIG_FILE &
#SERVER_PID=$!

# Give server time to start
#sleep 1

# ---------------------------
# Run tester
# ---------------------------
echo "[*] Running tester with URL: $URL"
$TESTER_EXEC $URL

# ---------------------------
# Stop server
# ---------------------------
#echo "[*] Stopping webserv (PID $SERVER_PID)"
#kill $SERVER_PID
#wait $SERVER_PID 2>/dev/null

echo "[*] Done"

