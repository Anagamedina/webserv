#!/usr/bin/env python3
"""
Manual test to understand CGI execution flow
"""
import subprocess
import os
import sys

# Test 1: Can we execute the script directly?
print("=== Test 1: Direct execution ===")
result = subprocess.run(['/usr/bin/python3', 'www/cgi-bin/test.py'], 
                       capture_output=True, text=True, cwd='/home/carles/Documents/42bcn/webserv')
print(f"Return code: {result.returncode}")
print(f"Stdout:\n{result.stdout}")
print(f"Stderr:\n{result.stderr}")

print("\n=== Test 2: Execution with environment variables ===")
env = os.environ.copy()
env['REQUEST_METHOD'] = 'GET'
env['QUERY_STRING'] = 'foo=bar'
env['GATEWAY_INTERFACE'] = 'CGI/1.1'
result = subprocess.run(['/usr/bin/python3', 'www/cgi-bin/echo_env.py'], 
                       capture_output=True, text=True, cwd='/home/carles/Documents/42bcn/webserv',
                       env=env)
print(f"Return code: {result.returncode}")
print(f"Stdout:\n{result.stdout[:500]}")
if result.stderr:
    print(f"Stderr:\n{result.stderr}")

print("\n=== Test 3: Simulating pipe communication ===")
import tempfile
proc = subprocess.Popen(['/usr/bin/python3', 'www/cgi-bin/test.py'],
                       stdout=subprocess.PIPE,
                       stdin=subprocess.PIPE,
                       stderr=subprocess.PIPE,
                       cwd='/home/carles/Documents/42bcn/webserv',
                       env=env)
stdout, stderr = proc.communicate(timeout=5)
print(f"Return code: {proc.returncode}")
print(f"Stdout:\n{stdout.decode()}")
if stderr:
    print(f"Stderr:\n{stderr.decode()}")
