import threading
import socket
import time
import sys

# Configuration
HOST = "127.0.0.1"
PORT = 8080
NUM_THREADS = 50
REQUESTS_PER_THREAD = 20

def send_request(thread_id):
    success_count = 0
    fail_count = 0
    
    for i in range(REQUESTS_PER_THREAD):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect((HOST, PORT))
            
            # Simple GET request
            request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            s.sendall(request.encode())
            
            data = s.recv(4096)
            if b"200 OK" in data:
                success_count += 1
            else:
                fail_count += 1
                # print(f"Thread {thread_id} req {i}: unexpected status")
            
            s.close()
        except Exception as e:
            fail_count += 1
            print(f"Thread {thread_id} req {i} error: {e}")
            
    return success_count, fail_count

threads = []
results = []

print(f"Starting stress test: {NUM_THREADS} threads, {REQUESTS_PER_THREAD} requests/thread...")
start_time = time.time()

class TestThread(threading.Thread):
    def run(self):
        s, f = send_request(self.name)
        results.append((s, f))

for i in range(NUM_THREADS):
    t = TestThread()
    threads.append(t)
    t.start()
    
for t in threads:
    t.join()
    
end_time = time.time()
duration = end_time - start_time

total_success = sum(r[0] for r in results)
total_fail = sum(r[1] for r in results)
total_requests = total_success + total_fail

print(f"Test completed in {duration:.2f} seconds.")
print(f"Total Requests: {total_requests}")
print(f"Successful: {total_success}")
print(f"Failed: {total_fail}")
print(f"Requests/sec: {total_requests/duration:.2f}")

if total_fail == 0:
    print("SUCCESS: server handled all requests.")
    sys.exit(0)
else:
    print("FAILURE: some requests failed.")
    sys.exit(1)
