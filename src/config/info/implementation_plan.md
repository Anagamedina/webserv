# Implementation Plan - Advanced Config Validation

Beyond the basic fixes, we need to ensure the parser is robust against malformed configuration files by enforcing strict rules.

## User Review Required

> [!IMPORTANT]
> **Strict Validation**: I will implement an "Unknown Directive" check. This means ANY word not recognized by the parser will throw an error instead of being ignored.

> [!CAUTION]
> **Argument Count Verification**: Directives like `root`, `listen`, and `client_max_body_size` will now throw an error if they receive more than one argument.

## Proposed Changes

### `src/config`

#### [MODIFY] [ConfigParser.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigParser.cpp)
- **`parseSingleServerBlock`**:
    - Add a final `else` clause at the end of the directive chain to throw `ConfigException("Unknown directive in server block")`.
    - Check `tokens.size()` for each directive to ensure exactly the expected number of arguments are provided.
- **`parseLocationBlock`**:
    - Add a final `else` clause at the end of the directive chain to throw `ConfigException("Unknown directive in location block")`.
    - Enforce argument counts for `root`, `autoindex`, `allow_methods`, etc.
- **`preprocessConfigFile`**:
    - Consider adding a check for text found *outside* of `server` blocks.

#### [MODIFY] [ConfigUtils.cpp](file:///home/daruuu/CLionProjects/webserv/src/config/ConfigUtils.cpp)
- Add or verify helpers for checking path existence or execution permissions (for CGI).

## Verification Plan

### Automated Tests
- Test with `unknown_directive.conf` (e.g., `server { random_word; }`) and expect failure.
- Test with `extra_args.conf` (e.g., `root /var /tmp;`) and expect failure.
- Test with `missing_args.conf` (e.g., `listen;`) and expect failure.

### Manual Verification
- Re-run existing `tester.conf` to ensure no regressions were introduced.
