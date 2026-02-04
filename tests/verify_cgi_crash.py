import subprocess
import time
import requests
import sys

def main():
    print("Starting server...")
    server_proc = subprocess.Popen(["./webserver"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        time.sleep(1) # Wait for server to start
        
        # Test 1: Crash
        print("Test 1: Requesting crashing CGI...")
        try:
            resp = requests.get("http://localhost:8080/cgi-bin/crash.py")
            print(f"Status Code: {resp.status_code}")
            if resp.status_code == 502:
                print("PASS: Got 502 Bad Gateway")
            else:
                print(f"FAIL: Expected 502, got {resp.status_code}")
                # We expect 502 if my fix works. If it wasn't fixed, it would likely be 200 or 500.
        except Exception as e:
            print(f"Request failed: {e}")

        # Test 2: Success
        print("\nTest 2: Requesting valid CGI...")
        try:
            resp = requests.get("http://localhost:8080/cgi-bin/hello.py")
            print(f"Status Code: {resp.status_code}")
            print(f"Body: {resp.text}")
            if resp.status_code == 200 and "Hello World" in resp.text:
                print("PASS: Got 200 OK and Hello World")
            else:
                print(f"FAIL: Expected 200/Hello World, got {resp.status_code}/{resp.text}")
        except Exception as e:
            print(f"Request failed: {e}")

        
    finally:
        if server_proc.poll() is None:
            server_proc.terminate()
        stdout, stderr = server_proc.communicate()
        if server_proc.returncode != 0:
            print(f"Server exited with code {server_proc.returncode}")
            print("STDOUT:", stdout)
            print("STDERR:", stderr)

if __name__ == "__main__":
    main()
