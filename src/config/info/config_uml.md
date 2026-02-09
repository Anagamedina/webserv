# Configuration Component UML

This diagram illustrates the structure and relationships of the configuration parsing components in `src/config`.

```mermaid
classDiagram
    class ConfigParser {
        -string config_file_path_
        -string clean_file_str_
        -size_t servers_count_
        -vector~string~ raw_server_blocks_
        -vector~ServerConfig~ servers_
        +parse()
        +getServers() vector~ServerConfig~
        -preprocessConfigFile() string
        -loadServerBlocks()
        -parseAllServerBlocks()
        -parseSingleServerBlock(string) ServerConfig
    }

    class ServerConfig {
        -int listen_port_
        -string host_address_
        -string server_name_
        -string root_
        -vector~string~ indexes_
        -size_t max_body_size_
        -map~int, string~ error_pages_
        -vector~LocationConfig~ locations_
        -bool autoindex_
        -int redirect_code_
        -string redirect_url_
        +addLocation(LocationConfig)
        +print()
    }

    class LocationConfig {
        -string path_
        -string root_
        -vector~string~ indexes_
        -vector~string~ allowed_methods_
        -bool autoindex_
        -string upload_store_
        -int redirect_code_
        -string redirect_url_
        -map~string, string~ cgi_handlers_
        +isMethodAllowed(string) bool
        +print()
    }

    class ConfigUtils-Namespace {
        +trimLine(string) string
        +removeComments(string)
        +tokenize(string) vector~string~
        +stringToInt(string) int
        +isValidPath(string) bool
    }

    ConfigParser "1" *-- "*" ServerConfig : contains
    ServerConfig "1" *-- "*" LocationConfig : contains
    ConfigParser ..> ConfigUtils : uses
    ServerConfig ..> ConfigUtils : uses
    LocationConfig ..> ConfigUtils : uses
```

## Component Roles

- **ConfigParser**: Responsible for high-level parsing of the `.conf` file. It splits the file into server blocks and orchestrates the population of `ServerConfig` objects.
- **ServerConfig**: Data structure representing a single `server { ... }` block. Holds server-wide configuration such as port, host, and a collection of locations.
- **LocationConfig**: Data structure representing a `location { ... }` block within a server. Defines behavior for specific URI paths.
- **ConfigUtils**: A collection of utility functions for string manipulation, file checks, and type conversion used throughout the configuration component.
