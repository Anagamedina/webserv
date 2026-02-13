#!/usr/bin/env python3
"""
Test that simulates exactly what CgiExecutor does
"""
import os
import subprocess
import sys

os.chdir('/home/carles/Documents/42bcn/webserv')

# Create pipes
r_in, w_in = os.pipe()
r_out, w_out = os.pipe()

script_path = os.path.abspath('www/cgi-bin/test.py')
interpreter_path = '/usr/bin/python3'

print(f"Script path: {script_path}")
print(f"Interpreter: {interpreter_path}")
print(f"Script exists: {os.path.exists(script_path)}")
print()

# Fork
pid = os.fork()

if pid == 0:
    # CHILD
    print(f"Child PID {os.getpid()}: Setting up pipes", file=sys.stderr)
    
    # Redirect stdin/stdout
    os.dup2(r_in, 0)  # STDIN
    os.dup2(w_out, 1) # STDOUT
    
    # Close pipe ends in child
    os.close(r_in)
    os.close(w_in)
    os.close(r_out)
    os.close(w_out)
    
    # Change to script directory
    script_dir = os.path.dirname(script_path)
    os.chdir(script_dir)
    print(f"Child changed to dir: {os.getcwd()}", file=sys.stderr)
    
    # Set environment
    env = os.environ.copy()
    env['GATEWAY_INTERFACE'] = 'CGI/1.1'
    env['REQUEST_METHOD'] = 'GET'
    env['QUERY_STRING'] = ''
    env['SCRIPT_FILENAME'] = script_path
    env['SCRIPT_NAME'] = '/cgi-bin/test.py'
    env['REQUEST_URI'] = '/cgi-bin/test.py'
    env['CONTENT_LENGTH'] = '0'
    
    # Exec
    print(f"Child executing: {interpreter_path} {script_path}", file=sys.stderr)
    sys.stderr.flush()
    os.execve(interpreter_path, [interpreter_path, script_path], env)
    
    # Should not reach here
    print("EXEC FAILED!", file=sys.stderr)
    sys.exit(1)
else:
    # PARENT
    print(f"Parent PID {os.getpid()}: Child PID {pid}")
    
    # Close unused pipe ends
    os.close(r_in)
    os.close(w_out)
    
    # Close stdin pipe write end
    os.close(w_in)
    
    # Read from child
    print("Parent reading from child...")
    response = b""
    while True:
        try:
            chunk = os.read(r_out, 4096)
            if not chunk:
                print("Got EOF")
                break
            print(f"Got {len(chunk)} bytes")
            response += chunk
        except Exception as e:
            print(f"Error reading: {e}")
            break
    
    os.close(r_out)
    
    # Wait for child
    print("Parent waiting for child...")
    _, status = os.waitpid(pid, 0)
    print(f"Child exited with status: {status}")
    
    print("\n=== RESPONSE ===")
    print(response.decode())
