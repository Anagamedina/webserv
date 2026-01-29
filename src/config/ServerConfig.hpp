#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

struct LocationConfig {
    std::string path;
    std::string root;
    std::string index;
    bool autoindex;
    std::set<std::string> accepted_methods; // GET, POST, DELETE
    std::string redirect_url; // For HTTP redirection
    // Handle uploads
    bool upload_enabled;
    std::string upload_path;
    // CGI
    std::map<std::string, std::string> cgi_handlers; // extension -> interpreter path
    size_t client_max_body_size; // 0 means default (inherit)

    LocationConfig() : autoindex(false), upload_enabled(false), client_max_body_size(0) {}
};

struct ServerBlock {
    int port;
    std::string host;
    std::vector<std::string> server_names;
    std::string root; // Default root for server
    std::string index; // Default index
    std::map<int, std::string> error_pages; // code -> file path
    size_t client_max_body_size; // in bytes
    std::vector<LocationConfig> locations;

    ServerBlock() : port(8080), host("127.0.0.1"), client_max_body_size(1024 * 1024) {} // Default 1MB
};

class ServerConfig {
public:
    std::vector<ServerBlock> servers;
};
