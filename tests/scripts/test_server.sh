#!/bin/bash

echo "Testing WebServer..."

echo "1. Basic GET request to port 8080"
curl -v http://localhost:8080/basic_test
echo -e "\n------------------------------------------------\n"

echo "2. Basic GET request to port 8081 (Second server)"
curl -v http://localhost:8081/second_port
echo -e "\n------------------------------------------------\n"

echo "3. Partial Request Parsing (Simulating slow client with netcat)"
# Sends "GET /partial HTTP/1.1", waits 1 sec, then sends "Host: localhost...", waits, then finishes.
(printf "GET /partial_test HTTP/1.1\r\n"; sleep 1; printf "Host: localhost:8080\r\n\r\n") | nc localhost 8080
echo -e "\n------------------------------------------------\n"

echo "Done."
