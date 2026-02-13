import socket
import time

def reproduce_issue():
    host = 'localhost'
    port = 8080

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        
        # Request 1: test.py
        print("Sending Request 1: test.py")
        req1 = "GET /cgi-bin/test.py HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n"
        s.sendall(req1.encode())
        
        # Read response 1
        print("Waiting for Response 1 (might take >4s)...")
        response1 = b""
        while b"\r\n\r\n" not in response1:
            chunk = s.recv(4096)
            if not chunk: break
            response1 += chunk
            
        print("Received headers for Response 1")
        # Read body
        if b"Content-Length:" in response1:
            headers, body = response1.split(b"\r\n\r\n", 1)
            cl = 0
            for line in headers.decode().split("\r\n"):
                if "Content-Length:" in line:
                    cl = int(line.split(":")[1].strip())
            
            print(f"Content-Length: {cl}")
            while len(body) < cl:
                chunk = s.recv(4096)
                if not chunk: break
                body += chunk
            response1 = headers + b"\r\n\r\n" + body
            
        print(f"Received Response 1: {len(response1)} bytes")
        # print first few lines of body
        print("Response 1 Body Start:")
        print(response1.split(b"\r\n\r\n")[1][:200].decode(errors='replace'))

        # Request 2: test.sh
        print("\nSending Request 2: test.sh")
        req2 = "GET /cgi-bin/test.sh HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n"
        s.sendall(req2.encode())
        
        # Read response 2
        print("Waiting for Response 2...")
        response2 = b""
        
        # We need to make sure we don't mix previous response data if any (though we read strictly by CL)
        
        while b"\r\n\r\n" not in response2:
            chunk = s.recv(4096)
            if not chunk: break
            response2 += chunk
            
        if b"Content-Length:" in response2:
             headers, body = response2.split(b"\r\n\r\n", 1)
             cl = 0
             for line in headers.decode().split("\r\n"):
                 if "Content-Length:" in line:
                     cl = int(line.split(":")[1].strip())
             while len(body) < cl:
                 chunk = s.recv(4096)
                 if not chunk: break
                 body += chunk
             response2 = headers + b"\r\n\r\n" + body

        print(f"Received Response 2: {len(response2)} bytes")
        print("Response 2 Body Start:")
        print(response2.split(b"\r\n\r\n")[1][:200].decode(errors='replace'))
        
        # Validation
        body1 = response1.split(b"\r\n\r\n")[1]
        body2 = response2.split(b"\r\n\r\n")[1]
        
        # Test.py output contains "Hello from Python CGI!"
        # Test.sh output contains "Hello from Bash CGI!"
        
        if b"Hello from Python CGI!" in body2:
            print("\nFAIL: Response 2 contains Python output!")
        elif b"Hello from Bash CGI!" in body2:
            print("\nSUCCESS: Response 2 contains Bash output.")
        else:
            print("\nUNKNOWN: Response 2 content unclear.")
            
        s.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    reproduce_issue()
