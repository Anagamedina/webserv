# Multi-Extension CGI Testing Guide

## Overview
Tests Python (.py), Shell (.sh), and Executable (.cgi) CGI scripts on the webserv project.
Configuration: `config/cgi_multi_test.conf`

## Quick Start

### 1. Compile and Start Server
```bash
make clean && make
./webserv config/cgi_multi_test.conf
```

### 2. Basic Tests

#### Test Shell Script (.sh)
```bash
# GET request with query parameters
curl "http://localhost:8080/cgi-bin/hello.sh?name=John&test=value"

# Expected: HTML page showing environment variables
```

#### Test Python Script (.py)
```bash
# GET request
curl "http://localhost:8080/cgi-bin/form_handler.py?field1=value1"

# POST request
curl -X POST \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=Alice&email=alice@example.com&message=Hello+CGI" \
  http://localhost:8080/cgi-bin/form_handler.py

# Expected: HTML page showing GET/POST parameters
```

#### Test Executable CGI (.cgi)
```bash
# GET request - returns JSON
curl "http://localhost:8080/cgi-bin/info.cgi?test=data"

# Pretty print JSON response
curl -s "http://localhost:8080/cgi-bin/info.cgi" | python3 -m json.tool

# Expected: JSON with environment and system info
```

### 3. Browser Tests

Open in browser:
- http://localhost:8080/cgi-bin/hello.sh?test=browser
- http://localhost:8080/cgi-bin/form_handler.py
- http://localhost:8080/cgi-bin/info.cgi

The form_handler.py page includes a test form for POST submissions.

## Advanced Tests

### Test Request Methods
```bash
# Test allowed methods (GET, POST)
curl -X GET http://localhost:8080/cgi-bin/hello.sh     # Should work
curl -X POST http://localhost:8080/cgi-bin/hello.sh    # Should work
curl -X DELETE http://localhost:8080/cgi-bin/hello.sh  # Should return 405
```

### Test Content Types
```bash
# POST with JSON (Python script will read stdin)
curl -X POST \
  -H "Content-Type: application/json" \
  -d '{"key":"value","number":42}' \
  http://localhost:8080/cgi-bin/form_handler.py
```

### Test Large POST Data
```bash
# Generate 1KB of data
curl -X POST \
  -d "$(python3 -c 'print("data=" + "A" * 1000)')" \
  http://localhost:8080/cgi-bin/form_handler.py
```

### Test HTTP Headers
```bash
# Send custom headers (will appear as HTTP_* environment variables)
curl -H "X-Custom-Header: test-value" \
     -H "X-Request-ID: 12345" \
     http://localhost:8080/cgi-bin/hello.sh
```

## Verification Checklist

### ✅ Functionality
- [ ] Shell scripts execute correctly (.sh)
- [ ] Python scripts execute correctly (.py)
- [ ] Executable scripts work (.cgi)
- [ ] GET parameters parsed correctly (QUERY_STRING)
- [ ] POST data received via stdin
- [ ] Environment variables set correctly
- [ ] HTTP headers passed as HTTP_* variables
- [ ] Status codes returned properly (200, 404, 500)
- [ ] Content-Type headers respected

### ✅ 42 School Compliance
- [ ] No zombie processes (check with `ps aux | grep defunct`)
- [ ] Non-blocking operation (server responsive during CGI)
- [ ] Proper fork/exec usage
- [ ] Pipe handling with proper closure
- [ ] Timeout enforcement (test with slow script)
- [ ] All used functions in allowed list

### ✅ Error Handling
- [ ] Missing script returns 404
- [ ] Script without execute permission returns 500
- [ ] Invalid interpreter returns 500
- [ ] Timeout (>5s) returns 504
- [ ] Large body (>client_max_body_size) returns 413

## Monitoring and Debugging

### Check for Zombie Processes
```bash
# Real-time monitoring (should show no defunct processes)
watch -n 1 'ps aux | grep defunct'

# Or single check
ps aux | grep webserv
```

### Monitor Server Logs
```bash
# Server outputs to stdout/stderr
./webserv config/cgi_multi_test.conf
```

### Test CGI Scripts Directly
```bash
# Test without server (debugging)
cd www/cgi-bin

# Test shell script
QUERY_STRING="test=value" REQUEST_METHOD="GET" ./hello.sh

# Test Python script
echo "name=test" | CONTENT_LENGTH=9 REQUEST_METHOD="POST" ./form_handler.py

# Test .cgi script
REQUEST_METHOD="GET" ./info.cgi
```

### Stress Testing
```bash
# Multiple simultaneous requests
for i in {1..10}; do
  curl -s "http://localhost:8080/cgi-bin/info.cgi?req=$i" &
done
wait

# Verify no zombies after burst
ps aux | grep defunct
```

## Expected Output Examples

### hello.sh
```html
<!DOCTYPE html>
<html>...
<p><strong>Request Method:</strong> GET</p>
<p><strong>Query String:</strong> test=value</p>
...
```

### form_handler.py (GET)
```html
<!DOCTYPE html>
<html>...
<h2>GET Parameters (Query String):</h2>
<ul>
<li><strong>field1:</strong> value1</li>
</ul>
...
```

### info.cgi
```json
{
  "status": "success",
  "message": "CGI script executed successfully via .cgi extension",
  "environment": {
    "cgi_version": "CGI/1.1",
    "request_method": "GET",
    ...
  },
  ...
}
```

## Troubleshooting

### Script returns 500
- Check execute permissions: `ls -la www/cgi-bin/`
- Verify shebang line is correct
- Test interpreter path: `which python3`, `which bash`

### No output / timeout
- Check script for infinite loops
- Verify script outputs headers (Content-Type, blank line)
- Check for syntax errors in script

### Zombie processes
- Verify waitpid() is being called (should be automatic)
- Check ServerManager::run() implementation

### POST data not received
- Check Content-Length header
- Verify stdin is being read in script
- Test with curl -v to see full request

## Performance Benchmarks

```bash
# Using Apache Bench (if available)
ab -n 100 -c 10 http://localhost:8080/cgi-bin/info.cgi

# Using hey (if available)  
hey -n 100 -c 10 http://localhost:8080/cgi-bin/info.cgi
```

## Cleanup

```bash
# Stop server: Ctrl+C

# Check for lingering processes
ps aux | grep webserv
pkill webserv

# Verify no zombies remain
ps aux | grep defunct
```
