# Host Binding Test Instructions

Follow these steps to verify the host binding implementation.

## 1. Start the Server with Test Config

Run the server using the provided `host_test.conf` file:

```bash
./webserver host_test.conf
```

## 2. Verify Bindings

Open another terminal and check the listening ports using `ss` or `netstat`. You should see specific IP bindings.

```bash
ss -lptn | grep webserver
```

**Expected Output (approximate):**
```
LISTEN 0      128        127.0.0.1:8081      0.0.0.0:*    users:(("webserver",pid=1234,fd=X))
LISTEN 0      128        127.0.0.1:8082      0.0.0.0:*    users:(("webserver",pid=1234,fd=Y))
LISTEN 0      128          0.0.0.0:8083      0.0.0.0:*    users:(("webserver",pid=1234,fd=Z))
LISTEN 0      128          0.0.0.0:8084      0.0.0.0:*    users:(("webserver",pid=1234,fd=W))
```

- **Port 8081**: Bound strictly to `127.0.0.1`. External connections (e.g., from another machine) to this port should fail.
- **Port 8082**: Bound to `localhost` (resolves to `127.0.0.1`).
- **Port 8083**: Bound to `0.0.0.0` (all interfaces). Accessible from anywhere.
- **Port 8084**: Bound to `0.0.0.0` (default when only port is specified).

## 3. Verify Connections

Test connectivity using `curl`.

### Local Access (Should Work)
```bash
curl -I http://127.0.0.1:8081
curl -I http://localhost:8082
curl -I http://127.0.0.1:8083
curl -I http://127.0.0.1:8084
```

### External Access Simulation
If you know your LAN IP (e.g., `192.168.1.X`), try connecting to it from another device or using the IP itself:

```bash
# Get your LAN IP
ip addr show | grep inet

# Test (assume IP is 192.168.1.50)
curl -I http://192.168.1.50:8081  # FAIL: Connection refused (bound to loopback only)
curl -I http://192.168.1.50:8083  # PASS: 200 OK (bound to all interfaces)
```

## 4. Automated Verification Script

You can also run the provided python script which automates these checks (excluding external access):

```bash
python3 verify_host_binding.py
```
