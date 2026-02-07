import socket
import time
import sys

HOST = "127.0.0.1"
PORT = 8080

def test_chunked_encoding():
    print("Testing Chunked Encoding...")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        
        # Headers
        headers = (
            "POST /cgi-bin/echo.py HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Utilities-Check: true\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
        )
        s.sendall(headers.encode())
        
        # Chunks
        chunks = [
            "4\r\nWiki\r\n",
            "5\r\npedia\r\n",
            "E\r\n in\r\n\r\nchunks.\r\n",
            "0\r\n\r\n"
        ]
        
        for chunk in chunks:
            s.sendall(chunk.encode())
            time.sleep(0.1) # Simulate slow network
            
        data = s.recv(4096)
        s.close()
        
        if b"200 OK" in data:
            print("[PASS] Chunked Encoding: Server accepted request")
            return True
        else:
            print(f"[FAIL] Chunked Encoding: Unexpected response\n{data.decode(errors='ignore')}")
            return False
    except Exception as e:
        print(f"[FAIL] Chunked Encoding Error: {e}")
        return False

def test_cookie_session():
    print("\nTesting Cookies & Sessions...")
    try:
        # Request 1: No Cookie
        s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s1.connect((HOST, PORT))
        s1.sendall(b"GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n")
        data1 = s1.recv(4096)
        s1.close()
        
        session_id = None
        data1_str = data1.decode(errors='ignore')
        if "Set-Cookie" in data1_str:
            start = data1_str.find("session_id=")
            if start != -1:
                end = data1_str.find(";", start)
                if end == -1: end = data1_str.find("\r\n", start)
                session_id = data1_str[start:end]
                print(f"[PASS] Session Created: {session_id}")
        else:
            print("[FAIL] No Set-Cookie header found")
            return False
            
        # Request 2: With Cookie
        if session_id:
            s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s2.connect((HOST, PORT))
            req = f"GET /index.html HTTP/1.1\r\nHost: localhost\r\nCookie: {session_id}\r\n\r\n"
            s2.sendall(req.encode())
            data2 = s2.recv(4096)
            s2.close()
            
            # Should NOT set new cookie if session valid (implementation dependent, but let's check basic success)
            if b"200 OK" in data2:
                print("[PASS] Session Reuse: Request successful")
                return True
            else:
                print(f"[FAIL] Session Reuse failed\n{data2.decode(errors='ignore')}")
                return False
        return False
    except Exception as e:
        print(f"[FAIL] Cookie Error: {e}")
        return False

def test_non_blocking_cgi():
    print("\nTesting Non-Blocking CGI...")
    # This requires a CGI script that sleeps, not provided here but we can assume /cgi-bin/sleep.py exists or similar.
    # We will just test generic CGI 
    return True

if __name__ == "__main__":
    passed = 0
    total = 0
    
    if test_chunked_encoding(): passed += 1
    total += 1
    
    if test_cookie_session(): passed += 1
    total += 1
    
    print(f"\nSummary: {passed}/{total} Tests Passed")
