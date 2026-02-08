#!/usr/bin/env bash
set -e

BASE="http://localhost:8080/cgi-bin"

echo "== GET with query"
curl -v "${BASE}/echo_env.py?foo=bar"

echo "== POST with body"
curl -v -X POST --data "hello=world" "${BASE}/echo_body.py"

echo "== No Content-Length from CGI"
curl -v "${BASE}/no_length.py"

echo "== Redirect"
curl -v "${BASE}/redirect.py"

echo "== Status 500"
curl -v "${BASE}/status_500.py"
