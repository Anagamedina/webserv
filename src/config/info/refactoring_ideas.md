# Refactoring & Clean Code Proposals

## Modularization Strategy

The `ConfigParser` class has grown significantly. To improve readability and maintainability, we should break down the largest functions into smaller, single-responsibility units.

### 1. `parseServer` Refactoring
Currently, `parseServer` handles iteration, tokenization, and directive branching for everything.
**Proposal:** Split into a main loop that delegates to specific handlers.

```cpp
// New structure
void ConfigParser::parseServer(const std::string& blockContent) {
    // ... setup ...
    while (getline(ss, line)) {
        // ... tokenization ...
        std::string directive = tokens[0];
        if (directive == "listen") handleListen(tokens, server);
        else if (directive == "location") handleLocation(tokens, server, ss);
        // ... etc
    }
}
```

### 2. `parseLocation` Extraction
The logic for parsing `location` blocks is nested inside `parseServer`.
**Proposal:** Extract to `ConfigParser::parseLocation(const std::string& path, std::stringstream& ss)`.

---

## Renaming for Clarity (Longest Functions)

Here are suggestions to rename the complex functions to better reflect their "Clean Code" intent.

### `extractRawBlocks`
*Current:* Iterates string, finds matching braces, extracts separate server blocks.
*Suggestions:*
1.  `segmentConfigFile` - Describes the action of breaking the file into segments.
2.  `extractServerConfigBlocks` - Very explicit about *what* is being extracted.
3.  `splitContentIntoBlocks` - Generic but clear.

### `CleanFileConfig`
*Current:* Reads file, removes comments, trims lines, normalizes spaces.
*Suggestions:*
1.  `preprocessConfiguration` - Standard term for preparing input.
2.  `sanitizeConfigContent` - Implies cleaning up (comments, whitespace).
3.  `loadAndCleanConfigFile` - Describes exactly the two steps it does.

### `ValidateCurlyBrackets`
*Current:* Checks if braces are balanced.
*Suggestions:*
1.  `validateBraceBalance` - More technical/precise.
2.  `checkScopeIntegrity` - Higher level description.
3.  `verifyStructuralIntegrity` - Sounds robust.

### `parseServer`
*Current:* Parses a single server block string into a ServerConfig object.
*Suggestions:*
1.  `buildServerConfig` - Describes the output (building an object).
2.  `processServerBlock` - Standard processing term.
3.  `parseServerDirective` - If we consider the whole block a directive (less accurate).

### `ValidateFilePermissions` / `ValidateFileExtension`
*Suggestions:*
1.  `canReadFile` / `hasValidExtension` - Boolean phrasing is often preferred for checks.
2.  `ensureFileAccessible` - If it were void and threw exception.

---

## Recommended Roadmap

1.  **Rename Phase**: Apply the new names to existing functions to clarify intent.
2.  **Extraction Phase**: 
    - Move `location` parsing logic out of `parseServer`.
    - Move specific directive handlers (`listen`, `root`, etc.) to private helper methods.
3.  **Cleanup**: Update header files and organize methods (public vs private).

---

## Additional Improvements

### Class Design & Data Structures
1.  **`ServerConfig` & `LocationConfig` Consistency**:
    *   Currently, some getters return copies and others references. Aim for `const &` (const reference) everywhere for performance, unless it's a primitive type like `int`.
    *   **Proposal**: Review all getters in `ServerConfig` and `LocationConfig` to ensure consistent interface.

2.  **Encapsulation of `LocationConfig`**:
    *   Consider moving the "Add Method" logic validation inside `LocationConfig` itself.
    *   Instead of `loc.addMethod(str)`, use `loc.addAllowedMethod(str)` which throws internal exceptions if the method is invalid (e.g., distinct from standard HTTP methods if you want strictness).

### Error Handling
1.  **Centralized Validation**:
    *   Currently, validation is scattered inside `parseServer`.
    *   **Proposal**: Create a `ConfigValidator` class or static methods in `ConfigUtils` for specific checks (e.g., `isValidAutoIndexValue`, `isValidRedirectCode`).
    *   This keeps the parser focused on *structure* while the validator focuses on *semantics*.

2.  **Rich Exceptions**:
    *   `ConfigException` takes a string.
    *   **Proposal**: Enhance it or helper function to automatically append line numbers or context "Near token: [X]" to make debugging config files much easier for the end user.

### File Structure
1.  **Splitting `ConfigParser`**:
    *   If `ConfigParser.cpp` exceeds ~800 lines, it's a smell.
    *   **Proposal**: Move private helper methods (like the proposed `handleListen`, `handleLocation`) to a separate file `ConfigParser_handlers.cpp` (if using the same class) or a helper class.
    *   Alternatively, `ServerBlockParser` could be its own class that `ConfigParser` instantiates for each block.

### Consistency Check
*   **Variable Naming**: Ensure member variables always end in `_` (e.g., `server_name_`) or never do. Currently seems mixed or mostly compliant. Stick to one style rigidly.
*   **Enum Usage**: Use `enum class` (C++11) or namespaced enums (C++98 style `namespace config { enum ... }`) for things like `AutoIndex` state or `MethodType` to avoid string comparisons everywhere.

