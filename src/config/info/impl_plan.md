# Refactoring Configuration Parser Module

Plan to modularize and complete the `src/config/` directory with proper error handling, configuration structures, and parsing logic.

## User Review Required

> [!IMPORTANT]
> **Breaking Changes**: This refactor will modify the public API of `ConfigParser`:
> - `parse()` method will change from `const` to non-const (fixes critical bug)
> - Exception-based error handling will replace `cout` messages
> - `getServers()` method will be uncommented and enabled

> [!WARNING] 
> **Dependencies**: The following classes will be created and may need integration with other modules:
> - `ConfigException` - Will be used throughout the project for config errors
> - `ServerConfig` - Will be consumed by `ServerManager` / `TcpListener`
> - `LocationConfig` - Will be used by HTTP routing logic

## Proposed Changes

### Module: Configuration Parsing & Validation

This change focuses on completing the configuration parser module with proper structure and error handling.

---

#### [NEW] [ConfigException.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigException.hpp)

Create exception class for configuration errors:
- Inherits from `std::exception`
- Stores custom error message
- Provides `what()` method for error details
- Thread-safe with `throw()` specification

---

#### [NEW] [ConfigException.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigException.cpp)

Implementation of exception class methods.

---

#### [NEW] [LocationConfig.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/LocationConfig.hpp)

Structure to hold `location {}` block configuration:
- Path (e.g., `/`, `/upload`)
- Root directory
- Index files
- Allowed HTTP methods (GET, POST, DELETE)
- Autoindex flag
- Upload directory
- Redirection rules

---

#### [NEW] [LocationConfig.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/LocationConfig.cpp)

Implementation with getters, setters, and validation.

---

#### [MODIFY] [ServerConfig.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ServerConfig.hpp)

Complete the empty file with server block configuration:
- Listen port
- Host address
- Server names
- Max body size
- Error pages map
- Vector of `LocationConfig` objects

---

#### [NEW] [ServerConfig.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ServerConfig.cpp)

Implementation of ServerConfig methods.

---

#### [NEW] [ConfigUtils.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigUtils.hpp)

Utility functions for parsing:
- `trim()` - Remove whitespace
- `split()` - Split string by delimiter
- `isCommentOrEmpty()` - Check if line is comment or blank
- `findMatchingBrace()` - Find closing `}` for opening `{`

---

#### [NEW] [ConfigUtils.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigUtils.cpp)

Implementation of utility functions.

---

#### [MODIFY] [ConfigParser.hpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.hpp)

Changes:
- Remove `const` from `parse()` method (LINE 16)
- Change `unsigned int` to `size_t` for `serversCount_`
- Uncomment `servers_` member variable and `getServers()` method
- Add private parsing helper methods:
  - `readFileContent()`
  - `extractServerBlocks()`
  - `parseServerBlocks()`
  - `parseServerBlock()`
  - `parseLocationBlock()`
- Add utility methods declarations
- Add validation method `validateParsedConfig()`

---

#### [MODIFY] [ConfigParser.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp)

Major implementation changes:
- Replace `std::cout` with exceptions (LINES 23, 27, 77)
- Implement real `parse()` method with full parsing logic
- Implement `readFileContent()` - Read entire file
- Implement `extractServerBlocks()` - Extract `server {}` blocks
- Implement `parseServerBlocks()` - Parse each server block
- Implement `parseServerBlock()` - Parse one server configuration
- Implement `parseLocationBlock()` - Parse location configuration
- Implement `validateParsedConfig()` - Final validation

---

#### [MODIFY] [mainConfig.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/mainConfig.cpp)

Refactor test file:
- Remove unused includes (LINES 2, 4-5, 7)
- Remove `findFile()` and `getParentDirectory()` functions (move to ConfigUtils)
- Fix hardcoded path from `../../config/default.conf` to `config/default.conf` (LINE 84)
- Add try-catch exception handling
- Clean up commented code
- Add output showing parsed server count

---

#### [DELETE] [flow-of-parsing](file:///home/daruuu/CLionProjects/webserv/src/config/flow-of-parsing)

Content should be moved to module documentation or code comments. Not needed as standalone file.

---

#### [NEW] [README.md](file:///home/daruuu/CLionProjects/webserv/src/config/README.md)

Module documentation:
- Purpose of config module
- File structure overview
- Usage examples
- Configuration file format
- Error handling approach

---

## Verification Plan

### Automated Tests

Since no existing tests were found in the project, new tests will be created:

#### 1. **Unit Test for ConfigException**
```bash
# Create: tests/unit/test_config_exception.cpp
# Run: make test_config && ./bin/test_config_exception
```
Tests:
- Exception message storage
- `what()` returns correct string
- Exception can be caught as `std::exception`

#### 2. **Unit Test for ConfigUtils**
```bash
# Create: tests/unit/test_config_utils.cpp
# Run: make test_config && ./bin/test_config_utils
```
Tests:
- `trim()` removes leading/trailing whitespace
- `split()` correctly splits strings
- `isCommentOrEmpty()` detects comments and blank lines
- `findMatchingBrace()` finds correct closing brace

#### 3. **Integration Test for ConfigParser**
```bash
# Create: tests/integration/test_config_parser.cpp
# Run: make test_config && ./bin/test_config_parser
```
Tests:
- Parse valid `default.conf` successfully
- Detect invalid file extension (`.txt`)
- Detect missing file
- Parse multiple server blocks
- Parse location blocks within servers
- Validate error page configuration
- Reject invalid directives

### Manual Verification

#### 1. **Test with existing config file**
```bash
cd /home/daruuu/CLionProjects/webserv
make re
./webserver config/default.conf
# Expected: Should print "Loaded 1 server(s)" without errors
```

#### 2. **Test with invalid extension**
```bash
./webserver config/invalid.txt
# Expected: Should print error "Invalid file extension, expected .conf" and exit
```

#### 3. **Test with missing file**
```bash
./webserver config/nonexistent.conf
# Expected: Should print error "Cannot open config file" and exit
```

#### 4. **Test with no arguments (default config)**
```bash
./webserver
# Expected: Should use config/default.conf and print "Loaded 1 server(s)"
```

#### 5. **Verify parsed configuration**
Add debug output to `mainConfig.cpp` temporarily:
```bash
# After parsing, print server details
./webserver config/default.conf
# Expected output should show:
# - Port: 8080
# - Host: 127.0.0.1
# - Max body size: 1048576
# - Number of locations: 2
# - Location paths: /, /upload
```
