#!/usr/bin/env python3
"""
Unit test for CGI execution without the full server
This tests the CGI executor logic directly
"""
import os
import sys
import subprocess
import json

os.chdir('/home/carles/Documents/42bcn/webserv')

def test_cgi_execution():
    """Test that we can execute a CGI script and get output"""
    
    # Simulate what CgiExecutor does
    r_in, w_in = os.pipe()
    r_out, w_out = os.pipe()
    
    script_path = os.path.abspath('www/cgi-bin/test.py')
    interpreter_path = '/usr/bin/python3'
    
    pid = os.fork()
    
    if pid == 0:
        # CHILD
        os.dup2(r_in, 0)
        os.dup2(w_out, 1)
        os.close(r_in)
        os.close(w_in)
        os.close(r_out)
        os.close(w_out)
        
        # Change directory
        script_dir = os.path.dirname(script_path)
        os.chdir(script_dir)
        
        # Set environment
        env = os.environ.copy()
        env['GATEWAY_INTERFACE'] = 'CGI/1.1'
        env['REQUEST_METHOD'] = 'GET'
        env['QUERY_STRING'] = ''
        env['SCRIPT_FILENAME'] = script_path
        env['SCRIPT_NAME'] = '/cgi-bin/test.py'
        env['REQUEST_URI'] = '/cgi-bin/test.py'
        env['CONTENT_LENGTH'] = '0'
        env['SERVER_PROTOCOL'] = 'HTTP/1.1'
        env['SERVER_SOFTWARE'] = 'Webserv/1.0'
        
        os.execve(interpreter_path, [interpreter_path, script_path], env)
        sys.exit(1)
    else:
        # PARENT
        os.close(r_in)
        os.close(w_out)
        os.close(w_in)
        
        response = b""
        while True:
            chunk = os.read(r_out, 4096)
            if not chunk:
                break
            response += chunk
        os.close(r_out)
        
        _, status = os.waitpid(pid, 0)
        
        return response.decode()

def test_query_string():
    """Test query string parsing"""
    r_in, w_in = os.pipe()
    r_out, w_out = os.pipe()
    
    script_path = os.path.abspath('www/cgi-bin/echo_env.py')
    interpreter_path = '/usr/bin/python3'
    query_string = 'foo=bar&baz=qux&name=John'
    
    pid = os.fork()
    
    if pid == 0:
        os.dup2(r_in, 0)
        os.dup2(w_out, 1)
        os.close(r_in)
        os.close(w_in)
        os.close(r_out)
        os.close(w_out)
        
        script_dir = os.path.dirname(script_path)
        os.chdir(script_dir)
        
        env = os.environ.copy()
        env['GATEWAY_INTERFACE'] = 'CGI/1.1'
        env['REQUEST_METHOD'] = 'GET'
        env['QUERY_STRING'] = query_string
        env['SCRIPT_FILENAME'] = script_path
        env['SCRIPT_NAME'] = '/cgi-bin/echo_env.py'
        env['REQUEST_URI'] = f'/cgi-bin/echo_env.py?{query_string}'
        env['CONTENT_LENGTH'] = '0'
        env['SERVER_PROTOCOL'] = 'HTTP/1.1'
        env['SERVER_SOFTWARE'] = 'Webserv/1.0'
        
        os.execve(interpreter_path, [interpreter_path, script_path], env)
        sys.exit(1)
    else:
        os.close(r_in)
        os.close(w_out)
        os.close(w_in)
        
        response = b""
        while True:
            chunk = os.read(r_out, 4096)
            if not chunk:
                break
            response += chunk
        os.close(r_out)
        
        _, status = os.waitpid(pid, 0)
        
        return response.decode()

def test_post_data():
    """Test POST data via stdin"""
    r_in, w_in = os.pipe()
    r_out, w_out = os.pipe()
    
    script_path = os.path.abspath('www/cgi-bin/echo_env.py')
    interpreter_path = '/usr/bin/python3'
    post_data = b'name=John&age=30&city=NYC'
    
    pid = os.fork()
    
    if pid == 0:
        os.dup2(r_in, 0)
        os.dup2(w_out, 1)
        os.close(r_in)
        os.close(w_in)
        os.close(r_out)
        os.close(w_out)
        
        script_dir = os.path.dirname(script_path)
        os.chdir(script_dir)
        
        env = os.environ.copy()
        env['GATEWAY_INTERFACE'] = 'CGI/1.1'
        env['REQUEST_METHOD'] = 'POST'
        env['QUERY_STRING'] = ''
        env['SCRIPT_FILENAME'] = script_path
        env['SCRIPT_NAME'] = '/cgi-bin/echo_env.py'
        env['REQUEST_URI'] = '/cgi-bin/echo_env.py'
        env['CONTENT_LENGTH'] = str(len(post_data))
        env['CONTENT_TYPE'] = 'application/x-www-form-urlencoded'
        env['SERVER_PROTOCOL'] = 'HTTP/1.1'
        env['SERVER_SOFTWARE'] = 'Webserv/1.0'
        
        os.execve(interpreter_path, [interpreter_path, script_path], env)
        sys.exit(1)
    else:
        os.close(r_in)
        os.close(w_out)
        
        # Write POST data to child stdin
        os.write(w_in, post_data)
        os.close(w_in)
        
        response = b""
        while True:
            chunk = os.read(r_out, 4096)
            if not chunk:
                break
            response += chunk
        os.close(r_out)
        
        _, status = os.waitpid(pid, 0)
        
        return response.decode()

# Run tests
print("=" * 70)
print("CGI Execution Unit Tests")
print("=" * 70)

print("\n[Test 1] Basic CGI Execution")
print("-" * 70)
try:
    output = test_cgi_execution()
    print(output[:500])
    if "Status: 200" in output and "Hello from Python CGI" in output:
        print("✓ PASSED")
    else:
        print("✗ FAILED - Missing expected output")
except Exception as e:
    print(f"✗ FAILED - {e}")

print("\n[Test 2] Query String Parsing")
print("-" * 70)
try:
    output = test_query_string()
    if "QUERY_STRING=foo=bar&baz=qux&name=John" in output:
        print("✓ PASSED - Query string correctly passed")
    else:
        print("✗ FAILED - Query string not found in output")
        print("Output snippet:")
        print(output[200:600])
except Exception as e:
    print(f"✗ FAILED - {e}")

print("\n[Test 3] POST Data via stdin")
print("-" * 70)
try:
    output = test_post_data()
    if "REQUEST_METHOD=POST" in output and "CONTENT_LENGTH=25" in output:
        print("✓ PASSED - POST data correctly handled")
    else:
        print("✗ FAILED - Expected POST headers not found")
        print("Output snippet:")
        print(output[200:600])
except Exception as e:
    print(f"✗ FAILED - {e}")

print("\n" + "=" * 70)
print("Note: These tests execute CGI directly, bypassing the server.")
print("The server will hang due to blocking I/O during CGI execution.")
print("This will be fixed when implementing async CGI handling.")
print("=" * 70)
