#!/bin/bash
VALGRIND_OPTS="--leak-check=full --show-leak-kinds=all --track-fds=yes --trace-children=yes --error-exitcode=1"
CONFIG="./config/test_multi_server.conf"

# Kill any process on the port
kill -9 $(lsof -ti:$PORT) 2>/dev/null

# Run server with valgrind
echo "Starting server with Valgrind, using config $CONFIG"
valgrind $VALGRIND_OPTS ./webserver $CONFIG

# Save PID
SERVER_PID=$!

# Give server time to start
sleep 2

# Run tests (example with curl)
echo "Running tests..."
for i in {1..10}; do
    curl -s http://localhost:$PORT/ > /dev/null
    curl -s http://localhost:$PORT/test > /dev/null
    curl -s -X POST http://localhost:$PORT/post -d "test=data" > /dev/null
done

# Kill server
echo "Stopping server..."
kill -INT $SERVER_PID
wait $SERVER_PID

echo "Valgrind test complete. Check valgrind.log for results."
