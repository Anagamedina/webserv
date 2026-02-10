# Quick Test Reference Guide

## Building and Running Tests

### Build Unit Tests

```bash
cd ./webserv
mkdir -p build && cd build
cmake ..
make unit_tests
```

### Run All Tests

```bash
./tests/unit/unit_tests
```

**Expected Output:**

```
===============================================================================
All tests passed (115 assertions in 26 test cases)
```

### Run Specific Test Categories

```bash
# IP/Host validation tests only
./tests/unit/unit_tests "[config][validation][ip]"

# Location path validation tests
./tests/unit/unit_tests "[config][validation][location]"

# HTTP method validation tests
./tests/unit/unit_tests "[config][validation][methods]"

# Port validation tests
./tests/unit/unit_tests "[config][validation][port]"

# Body size validation tests
./tests/unit/unit_tests "[config][validation][bodysize]"

# Path existence checks
./tests/unit/unit_tests "[config][validation][pathcheck]"

# All integration tests
./tests/unit/unit_tests "[config][integration]"
```

### Verbose Test Output

```bash
# Show all test sections and assertions
./tests/unit/unit_tests -s

# Show successful test results too
./tests/unit/unit_tests -s -a
```

### Run Specific Test Case

```bash
# By exact name
./tests/unit/unit_tests "config::utils::isValidIPv4 - valid IPv4 addresses"

# By partial match
./tests/unit/unit_tests "isValidIPv4"
```

---

## Manual Testing with Config Files

### Test Invalid IP Address

```bash
cd /home/carles/Documents/42bcn/webserv

# Create test config
printf 'server {\n    listen 256.1.1.1:8080;\n    root /tmp;\n}\n' > /tmp/test_invalid_ip.conf

# Run webserver (should fail)
./webserver /tmp/test_invalid_ip.conf
```

**Expected Error:**

```
Error: Invalid IP address or hostname format: 256.1.1.1
```

### Test Invalid HTTP Method

```bash
# Create test config
printf 'server {\n    listen 8080;\n    location / {\n        limit_except PUT;\n        root /tmp;\n    }\n}\n' > /tmp/test_invalid_method.conf

# Run webserver (should fail)
./webserver /tmp/test_invalid_method.conf
```

**Expected Error:**

```
Error: HTTP method must be GET, POST, or DELETE: PUT
```

### Test Invalid Location Path

```bash
# Create test config
printf 'server {\n    listen 8080;\n    location api {\n        root /tmp;\n    }\n}\n' > /tmp/test_invalid_location.conf

# Run webserver (should fail)
./webserver /tmp/test_invalid_location.conf
```

**Expected Error:**

```
Error: Location path must start with '/' and not be empty: api
```

### Test Valid Configuration

```bash
# Create valid config
printf 'server {\n    listen 127.0.0.1:8080;\n    location / {\n        limit_except GET POST DELETE;\n        root /tmp;\n    }\n}\n' > /tmp/test_valid.conf

# Run webserver (should start successfully)
timeout 2 ./webserver /tmp/test_valid.conf
```

**Expected Output:**

```
Epoll created successfully (fd: 3)
Socket created (fd: 4)
Socket bound to port 8080
Listening for connections...
Server listening on port 8080
Server started. Waiting for events...
```

---

## Common Test Scenarios

### 1. IPv4 Validation

**Valid:**

- `127.0.0.1:8080`
- `0.0.0.0:80`
- `192.168.1.1:3000`

**Invalid:**

- `256.1.1.1:8080` (octet > 255)
- `192.168.1:8080` (too few octets)
- `192.168.1.1.1:8080` (too many octets)

### 2. Hostname Validation

**Valid:**

- `localhost:8080`
- `example.com:80`
- `my-server.local:3000`

**Invalid:**

- `-invalid.com:8080` (starts with hyphen)
- `example-.com:8080` (hyphen before dot)
- `example..com:8080` (consecutive dots)

### 3. Port Validation

**Valid:**

- `1` to `65535`

**Invalid:**

- `0` (too low)
- `65536` (too high)
- `-1` (negative)

### 4. Location Paths

**Valid:**

- `/`
- `/api`
- `/api/v1/users`

**Invalid:**

- `api` (no leading slash)
- `` (empty)
- `//api` (double slash)

### 5. HTTP Methods

**Valid:**

- `GET`
- `POST`
- `DELETE`

**Invalid:**

- `PUT` (not in whitelist)
- `PATCH` (not in whitelist)
- `get` (lowercase)
- `HEAD` (not in whitelist)

### 6. Body Size

**Valid:**

- `0` to `2147483647` (2GB max)
- With suffixes: `1K`, `10M`, `1G`

**Invalid:**

- `-1` (negative)
- `3G` (exceeds 2GB)
- `-100K` (negative with suffix)

---

## Debugging Failed Tests

### View Detailed Failure Information

```bash
./tests/unit/unit_tests -s 2>&1 | grep -A20 "FAILED"
```

### Check Specific Assertion

```bash
./tests/unit/unit_tests -s 2>&1 | grep -B5 "with expansion"
```

### Run Only Failed Test After Fix

```bash
# Example: re-run IP validation after fixing isValidIPv4
./tests/unit/unit_tests "isValidIPv4"
```

---

## Adding New Tests

### Unit Test Template

```cpp
TEST_CASE("config::utils::newFunction - description", "[config][validation][tag]")
{
    SECTION("Test case 1") {
        REQUIRE(config::utils::newFunction("valid_input") == true);
    }

    SECTION("Test case 2") {
        REQUIRE(config::utils::newFunction("invalid_input") == false);
    }
}
```

### Integration Test Template

```cpp
TEST_CASE("Integration: New validation", "[config][integration][tag]")
{
    SECTION("Valid configuration") {
        std::ofstream file("test_valid_new.conf");
        file << "server {\n"
             << "    listen 8080;\n"
             << "    new_directive valid_value;\n"
             << "}\n";
        file.close();

        ConfigParser parser("test_valid_new.conf");
        REQUIRE_NOTHROW(parser.parse());
        std::remove("test_valid_new.conf");
    }

    SECTION("Invalid configuration") {
        std::ofstream file("test_invalid_new.conf");
        file << "server {\n"
             << "    listen 8080;\n"
             << "    new_directive invalid_value;\n"
             << "}\n";
        file.close();

        ConfigParser parser("test_invalid_new.conf");
        REQUIRE_THROWS_AS(parser.parse(), ConfigException);
        std::remove("test_invalid_new.conf");
    }
}
```

---

## Continuous Integration

### Pre-Commit Check

```bash
#!/bin/bash
# Save as .git/hooks/pre-commit

cd build
make unit_tests
if ! ./tests/unit/unit_tests; then
    echo "❌ Tests failed! Commit aborted."
    exit 1
fi
echo "✅ All tests passed!"
```

### Make Test Target (Optional)

Add to `Makefile`:

```makefile
test: unit_tests
	@echo "Running unit tests..."
	@cd build && ./tests/unit/unit_tests
	@echo "✅ All tests passed!"

.PHONY: test
```

Usage:

```bash
make test
```

---

## Troubleshooting

### Tests Don't Compile

```bash
# Clean and rebuild
rm -rf build
mkdir build && cd build
cmake ..
make unit_tests
```

### "No such file or directory" for test executable

```bash
# Executable is in build/tests/unit/
cd build
./tests/unit/unit_tests

# Or use absolute path
/home/carles/Documents/42bcn/webserv/build/tests/unit/unit_tests
```

### "ConfigException not thrown"

- Check if validation function is actually called in parser
- Verify exception type matches `ConfigException`
- Use `-s` flag to see where test stops

### Compilation warnings about includes

- Ensure `#include "../common/namespaces.hpp"` uses correct relative path
- Check CMake include directories in `CMakeLists.txt`

---

## Performance Benchmarks

### Test Execution Speed

```bash
# Time all tests
time ./tests/unit/unit_tests
```

**Expected:** < 1 second for all 26 test cases

### Config Parsing Speed

```bash
# Time config file parsing
time ./webserver config/default.conf
```

---

## Coverage Report (Manual)

Run tests and verify coverage:

| Validation Category  | Test Cases | Status  |
| -------------------- | ---------- | ------- |
| IPv4 format          | 5          | ✅ Pass |
| IPv4 invalid         | 8          | ✅ Pass |
| Hostname valid       | 6          | ✅ Pass |
| Hostname invalid     | 8          | ✅ Pass |
| Host combined        | 5          | ✅ Pass |
| Port valid           | 4          | ✅ Pass |
| Port invalid         | 4          | ✅ Pass |
| Location valid       | 5          | ✅ Pass |
| Location invalid     | 6          | ✅ Pass |
| HTTP methods valid   | 3          | ✅ Pass |
| HTTP methods invalid | 8          | ✅ Pass |
| Error codes valid    | 4          | ✅ Pass |
| Error codes invalid  | 4          | ✅ Pass |
| Body size valid      | 6          | ✅ Pass |
| Body size invalid    | 5          | ✅ Pass |
| Path checks          | 6          | ✅ Pass |
| Integration tests    | 10         | ✅ Pass |

**Total:** 26 test cases, 115 assertions, 100% pass rate

---

## Quick Commands Cheatsheet

```bash
# Build everything
make && cd build && cmake .. && make unit_tests

# Run all tests
./tests/unit/unit_tests

# Run tests with full output
./tests/unit/unit_tests -s -a

# Run only IP tests
./tests/unit/unit_tests "[ip]"

# Test invalid config manually
./webserver /tmp/test_invalid.conf

# Clean and rebuild
make fclean && make
```
