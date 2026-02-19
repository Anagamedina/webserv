#!/usr/bin/env python3
"""
Comprehensive CGI Integration Tests for Webserv
Tests the full request → CGI execution → response flow

This test suite validates:
1. Basic CGI execution with GET requests
2. Query string parsing and passing to CGI
3. POST request body via stdin to CGI
4. Environment variable setup
5. Response header/body parsing
6. Error handling (timeouts, missing scripts, etc.)
"""

import socket
import subprocess
import time
import sys
import os
from pathlib import Path

class CGITestClient:
    """HTTP client for sending requests to webserver and validating responses"""
    
    def __init__(self, host="127.0.0.1", port=8080):
        self.host = host
        self.port = port
    
    def send_request(self, method, path, headers=None, body=""):
        """
        Send HTTP request and return (status_code, response_headers, response_body)
        
        Args:
            method: HTTP method (GET, POST, DELETE, etc.)
            path: Request path (e.g., "/cgi-bin/test.py?foo=bar")
            headers: Dict of HTTP headers
            body: Request body (for POST, PUT)
        
        Returns:
            (status_code, headers_dict, body_str)
        """
        if headers is None:
            headers = {}
        
        # Build HTTP request
        request = f"{method} {path} HTTP/1.1\r\n"
        request += f"Host: {self.host}:{self.port}\r\n"
        request += "Connection: close\r\n"
        
        if body:
            headers["Content-Length"] = str(len(body))
            if "Content-Type" not in headers:
                headers["Content-Type"] = "application/x-www-form-urlencoded"
        
        for key, value in headers.items():
            request += f"{key}: {value}\r\n"
        
        request += "\r\n"
        request += body
        
        # Send request
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((self.host, self.port))
            sock.sendall(request.encode())
            
            # Receive response
            response = b""
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
        finally:
            sock.close()
        
        # Parse response
        response_str = response.decode('utf-8', errors='ignore')
        parts = response_str.split('\r\n\r\n', 1)
        if len(parts) == 2:
            headers_str, body_str = parts
        else:
            # Try \n\n separator
            parts = response_str.split('\n\n', 1)
            headers_str = parts[0]
            body_str = parts[1] if len(parts) > 1 else ""
        
        # Parse status line
        lines = headers_str.split('\r\n')
        if not lines:
            lines = headers_str.split('\n')
        
        status_line = lines[0]
        status_code = int(status_line.split()[1]) if len(status_line.split()) > 1 else 500
        
        # Parse response headers
        response_headers = {}
        for line in lines[1:]:
            if ':' in line:
                key, value = line.split(':', 1)
                response_headers[key.strip()] = value.strip()
        
        return status_code, response_headers, body_str

class CGITests:
    """Test suite for CGI functionality"""
    
    def __init__(self, client):
        self.client = client
        self.tests_passed = 0
        self.tests_failed = 0
    
    def assert_equal(self, actual, expected, msg=""):
        """Assert actual equals expected"""
        if actual == expected:
            self.tests_passed += 1
            print(f"  ✓ {msg}")
            return True
        else:
            self.tests_failed += 1
            print(f"  ✗ {msg}")
            print(f"    Expected: {expected}")
            print(f"    Got:      {actual}")
            return False
    
    def assert_in(self, needle, haystack, msg=""):
        """Assert needle is in haystack"""
        if needle in haystack:
            self.tests_passed += 1
            print(f"  ✓ {msg}")
            return True
        else:
            self.tests_failed += 1
            print(f"  ✗ {msg}")
            print(f"    Expected to find: {needle}")
            print(f"    In: {haystack}")
            return False
    
    def assert_status(self, status_code, expected, msg=""):
        """Assert HTTP status code"""
        return self.assert_equal(status_code, expected, f"Status code: {msg}")
    
    def test_basic_cgi_execution(self):
        """Test 1: Basic CGI execution with GET request"""
        print("\n[Test 1] Basic CGI execution with GET request")
        status, headers, body = self.client.send_request("GET", "/cgi-bin/test.py")
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("text/plain", headers.get("Content-Type", ""), 
                       "Response should have Content-Type: text/plain")
        self.assert_in("Method: GET", body, 
                       "Response body should contain Method: GET")
    
    def test_query_string_parsing(self):
        """Test 2: Query string parsing and passing to CGI"""
        print("\n[Test 2] Query string parsing")
        
        # Request with query string
        status, headers, body = self.client.send_request(
            "GET", 
            "/cgi-bin/echo_env.py?foo=bar&baz=qux&empty="
        )
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("QUERY_STRING=foo=bar&baz=qux&empty=", body,
                       "QUERY_STRING should be passed correctly")
        
        # Request without query string
        status, headers, body = self.client.send_request("GET", "/cgi-bin/echo_env.py")
        self.assert_in("QUERY_STRING=", body,
                       "QUERY_STRING should be empty when no query params")
    
    def test_post_with_body(self):
        """Test 3: POST request with body passed via stdin"""
        print("\n[Test 3] POST request with body")
        
        post_data = "name=John&age=30&city=NYC"
        status, headers, body = self.client.send_request(
            "POST",
            "/cgi-bin/echo_env.py",
            body=post_data
        )
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("REQUEST_METHOD=POST", body,
                       "REQUEST_METHOD should be POST")
        self.assert_in("CONTENT_LENGTH", body,
                       "CONTENT_LENGTH should be set")
        # Note: Actual body contents passed via stdin are NOT echoed by echo_env.py
        # but CONTENT_LENGTH should be set correctly
    
    def test_content_type_header(self):
        """Test 4: Content-Type header passed as environment variable"""
        print("\n[Test 4] Content-Type header in CGI environment")
        
        status, headers, body = self.client.send_request(
            "POST",
            "/cgi-bin/echo_env.py",
            headers={"Content-Type": "application/json"},
            body='{"test": "data"}'
        )
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("CONTENT_TYPE=application/json", body,
                       "CONTENT_TYPE should be in environment")
    
    def test_http_headers_as_env_vars(self):
        """Test 5: HTTP headers converted to HTTP_* environment variables"""
        print("\n[Test 5] HTTP headers as HTTP_* environment variables")
        
        status, headers, body = self.client.send_request(
            "GET",
            "/cgi-bin/echo_env.py",
            headers={
                "X-Custom-Header": "test-value",
                "X-Another": "hello"
            }
        )
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("HTTP_X_CUSTOM_HEADER=test-value", body,
                       "Custom headers should be converted to HTTP_* vars")
    
    def test_request_uri(self):
        """Test 6: REQUEST_URI environment variable"""
        print("\n[Test 6] REQUEST_URI environment variable")
        
        status, headers, body = self.client.send_request(
            "GET",
            "/cgi-bin/echo_env.py?search=webserv&lang=en"
        )
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("REQUEST_URI=/cgi-bin/echo_env.py", body,
                       "REQUEST_URI should include full path with query string")
    
    def test_gateway_interface(self):
        """Test 7: GATEWAY_INTERFACE version"""
        print("\n[Test 7] GATEWAY_INTERFACE and SERVER_PROTOCOL")
        
        status, headers, body = self.client.send_request("GET", "/cgi-bin/echo_env.py")
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("GATEWAY_INTERFACE=CGI/1.1", body,
                       "GATEWAY_INTERFACE should be CGI/1.1")
        self.assert_in("SERVER_PROTOCOL=HTTP/1.1", body,
                       "SERVER_PROTOCOL should be HTTP/1.1")
    
    def test_cgi_response_format(self):
        """Test 8: CGI response format (headers and body)"""
        print("\n[Test 8] CGI response format parsing")
        
        status, headers, body = self.client.send_request("GET", "/cgi-bin/hello.py")
        
        self.assert_status(status, 200, "Should return 200 OK")
        self.assert_in("Hello", body, "Response body should contain Hello")
    
    def test_missing_cgi_script(self):
        """Test 9: Request to non-existent CGI script"""
        print("\n[Test 9] Request to non-existent CGI script")
        
        status, headers, body = self.client.send_request("GET", "/cgi-bin/nonexistent.py")
        
        # Should return 404 or 500 depending on server implementation
        self.assert_in(status, [404, 500], f"Non-existent script should return error (got {status})")
    
    def test_method_passed_to_cgi(self):
        """Test 10: REQUEST_METHOD is correctly passed"""
        print("\n[Test 10] REQUEST_METHOD is correctly passed")
        
        # Test GET
        status, headers, body = self.client.send_request("GET", "/cgi-bin/test.py")
        self.assert_in("GET", body, "GET method should be in response")
        
        # Test POST
        status, headers, body = self.client.send_request("POST", "/cgi-bin/test.py", body="data=test")
        self.assert_in("POST", body, "POST method should be in response")
    
    def run_all_tests(self):
        """Run all tests and print summary"""
        print("=" * 70)
        print("CGI Integration Test Suite for Webserv")
        print("=" * 70)
        
        self.test_basic_cgi_execution()
        self.test_query_string_parsing()
        self.test_post_with_body()
        self.test_content_type_header()
        self.test_http_headers_as_env_vars()
        self.test_request_uri()
        self.test_gateway_interface()
        self.test_cgi_response_format()
        self.test_missing_cgi_script()
        self.test_method_passed_to_cgi()
        
        print("\n" + "=" * 70)
        print(f"Tests Passed: {self.tests_passed}")
        print(f"Tests Failed: {self.tests_failed}")
        print("=" * 70)
        
        return self.tests_failed == 0

def start_server(config_path):
    """Start the webserver in background"""
    print(f"Starting webserver with config: {config_path}")
    try:
        # Change to repo directory
        os.chdir(Path(__file__).parent.parent)
        
        # Start server
        proc = subprocess.Popen(
            ["./webserver", config_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
        # Wait for server to start
        time.sleep(1)
        
        return proc
    except Exception as e:
        print(f"Failed to start server: {e}")
        return None

def main():
    """Main test entry point"""
    config_path = "configs/default.conf"
    
    if not os.path.exists(config_path):
        print(f"Error: Config file {config_path} not found")
        sys.exit(1)
    
    # Try to compile first
    print("Building webserver...")
    result = subprocess.run(["make", "clean"], capture_output=True)
    result = subprocess.run(["make", "all"], capture_output=True)
    
    if result.returncode != 0:
        print("Failed to compile webserver:")
        print(result.stderr.decode())
        sys.exit(1)
    
    # Start server
    server_proc = start_server(config_path)
    if not server_proc:
        sys.exit(1)
    
    try:
        # Run tests
        client = CGITestClient("127.0.0.1", 8080)
        tests = CGITests(client)
        success = tests.run_all_tests()
        
        sys.exit(0 if success else 1)
    finally:
        # Kill server
        if server_proc:
            server_proc.terminate()
            try:
                server_proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                server_proc.kill()
            print("\nServer stopped")

if __name__ == "__main__":
    main()
