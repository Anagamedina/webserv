import subprocess
import time
import socket
import sys
import os
import signal

WEBSERV_BIN = "./webserver"
CONFIG_FILE = "temp_config_test.conf"

def write_config(host, port):
    with open(CONFIG_FILE, "w") as f:
        f.write("server {\n")
        f.write(f"    listen {host}:{port};\n")
        f.write("    root ./www;\n")
        f.write("    index index.html;\n")
        f.write("}\n")

def run_server(timeout=2):
    # Start the server in the background
    # Ensure stdout/stderr are captured so we can check for startup errors
    process = subprocess.Popen([WEBSERV_BIN, CONFIG_FILE], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(timeout) # Give it a moment to start (and fail if invalid host)
    return process

def check_connection(target_ip, port, expected_success):
    result = False
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2)
        s.connect((target_ip, port))
        s.close()
        result = True
    except socket.error as e:
        result = False

    if expected_success and result:
        print(f"[PASS] Successfully connected to {target_ip}:{port}")
        return True
    elif expected_success and not result:
        print(f"[FAIL] Could not connect to {target_ip}:{port}")
        return False
    elif not expected_success and not result:
        print(f"[PASS] Correctly failed to connect to {target_ip}:{port}")
        return True
    else:
        print(f"[FAIL] Unexpectedly connected to {target_ip}:{port}")
        return False


def test_scenario(bind_host, bind_port, connect_ip, expected_success, description, timeout=2):
    print(f"\n--- Test: {description} ---")
    print(f"Config: host={bind_host}, port={bind_port}")
    write_config(bind_host, bind_port)
    
    server_process = run_server(timeout)
    
    # Check if server died immediately
    if server_process.poll() is not None:
        out, err = server_process.communicate()
        print(f"[FAIL] Server failed to start: {err.decode().strip()}")
        return False

    result = check_connection(connect_ip, bind_port, expected_success)
    
    try:
        os.kill(server_process.pid, signal.SIGTERM)
        server_process.wait()
    except OSError:
        pass
        
    return result

def main():
    if not os.path.exists(WEBSERV_BIN):
        print(f"Error: {WEBSERV_BIN} not found. Run make first.")
        sys.exit(1)

    all_passed = True

    # Test 1: Bind to 127.0.0.1, connect via 127.0.0.1 (Should succeed)
    if not test_scenario("127.0.0.1", 8081, "127.0.0.1", True, "Explicit Localhost Bind"):
        all_passed = False

    # Test 2: Bind to 0.0.0.0, connect via 127.0.0.1 (Should succeed)
    if not test_scenario("0.0.0.0", 8082, "127.0.0.1", True, "Any Interface Bind"):
        all_passed = False

    # Test 3: Bind to localhost, connect via 127.0.0.1 (Should succeed)
    if not test_scenario("localhost", 8083, "127.0.0.1", True, "Hostname 'localhost' Bind"):
        all_passed = False

    # Test 4: Invalid Host (Server should fail to start)
    print("\n--- Test: Invalid Hostname ---")
    write_config("invalid.host.name.test", 8084)
    # DNS timeout can be ~10s. We wait 12s to be safe.
    server_process = run_server(timeout=12)
    if server_process.poll() is not None:
         print("[PASS] Server correctly failed to start with invalid host")
    else:
         print("[FAIL] Server started with invalid host")
         try:
            os.kill(server_process.pid, signal.SIGTERM)
            out, err = server_process.communicate()
            print("--- Server Output ---")
            print(out.decode())
            print(err.decode())
            print("---------------------")
         except OSError:
            pass
         all_passed = False

    if os.path.exists(CONFIG_FILE):
        os.remove(CONFIG_FILE)

    if all_passed:
        print("\nAll tests passed!")
        sys.exit(0)
    else:
        print("\nSome tests failed.")
        sys.exit(1)

if __name__ == "__main__":
    main()
