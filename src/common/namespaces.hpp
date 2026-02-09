#ifndef WEBSERV_NAMESPACES_HPP
#define WEBSERV_NAMESPACES_HPP
#include <string>

namespace config {
namespace colors {
// Colors of text (foreground)
static const char* const reset = "\033[0m";
static const char* const bold = "\033[1m";
static const char* const underline = "\033[4m";

static const char* const black = "\033[30m";
static const char* const red = "\033[31m";
static const char* const green = "\033[32m";
static const char* const yellow = "\033[33m";
static const char* const blue = "\033[34m";
static const char* const magenta = "\033[35m";
static const char* const cyan = "\033[36m";
static const char* const white = "\033[37m";
} // namespace colors

namespace paths {
// static const std::string default_config_path = "config/file.conf";
static const std::string default_config_path = "config/default.conf";

static const std::string log_file_config = "../config/logs/config-clean.log";
static const std::string log_file_server = "../config/logs/server.log";
static const std::string log_file_block = "../config/logs/block.log";
static const std::string extension_file = ".conf";
} // namespace paths

namespace errors {
static const char* const invalid_extension = "Invalid file extension: ";
static const std::string cannot_open_file = "Cannot open config file: ";
static const std::string number_out_of_range = "Number out of range.";
static const std::string invalid_characters = "Invalid Characters in stringToInt().";
static const std::string invalid_num_args_return_directive =
    "Invalid number of arguments in 'return' directive";
static const std::string invalid_redirect_code = "Invalid redirect code.";
static const std::string missing_args_in_return = "Missing arguments after 'return' directive";

static const std::string invalid_min_num_args_upload_directive =
    "Missing path in 'upload_store' directive";
static const std::string invalid_max_num_args_upload_directive =
    "Too many arguments in 'upload_store' directive";
static const std::string invalid_characters_in_upload_directive =
    "Invalid characters in upload_store path: ";

static const std::string empty_path_in_upload_directive = "Empty path in 'upload_store' directive";
static const std::string invalid_port_range = "Invalid port: must be 1 - 65535";
static const std::string invalid_http_status_code = "Invalid HTTP status code.";
static const std::string invalid_autoindex = "autoindex must be 'on' or 'off'.";
static const std::string missing_args_in_index = "Missing arguments in 'index' directive.";
static const std::string invalid_new_location_block = "Invalid nested new 'location' directive.";
} // namespace errors

namespace section {
static const std::string server = "server";
static const std::string listen = "listen";
static const std::string server_name = "server_name";
static const std::string client_max_body_size = "client_max_body_size";
static const std::string location = "location";
static const std::string error_page = "error_page";
static const std::string root = "root";
static const std::string index = "index";
static const std::string autoindex = "autoindex";
static const std::string autoindex_on = "on";
static const std::string autoindex_off = "off";
static const std::string upload_bonus = "/uploads";
static const std::string uploads_bonus = "/uploads";
static const std::string methods = "methods";
static const std::string allow_methods = "allow_methods";
static const std::string limit_except = "limit_except";
static const std::string return_str = "return";

// 1. Exact match (prioridad más alta: coincide SOLO si la URI es idéntica)
// location = /exact/path { ... }
static const std::string exact_match_modifier = "=";

// 2. Preferential prefix (prefix más largo, pero detiene la búsqueda de regex si coincide)
// location ^~ /images/ { ... } → no chequea regex después
static const std::string preferential_prefix_modifier = "^~";

static const int default_return_code = 302;
static const int default_port = 80;
static const std::string default_host_name = "127.0.0.1";
static const size_t max_body_size = 1048576;
static const int max_port = 65535;
static const std::string method_get = "GET";
static const std::string method_head = "HEAD";
static const std::string cgi = "cgi";
static const std::string cgi_fast = "fastcgi_pass";
} // namespace section

enum ParserState {
    OUTSIDE_BLOCK,
    IN_SERVER,
    IN_LOCATION
};

namespace debug {
void debugConfigLog(const std::string& config_file_path);
}
} // namespace config

#endif
