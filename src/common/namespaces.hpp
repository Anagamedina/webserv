#ifndef WEBSERV_NAMESPACES_HPP
#define WEBSERV_NAMESPACES_HPP
#include <string>

namespace constants
{
	static const std::string default_config_path = "../config/default.conf";
	// static const std::string default_config_path = "../config/examples/nginx.conf";
	// static const std::string log_file = "../config/logs/config-clean.log";
	static const std::string log_file_output = "config_pretty.log";
	static const std::string extension_file = ".conf";
}

namespace colors
{
	// Colores de texto (foreground)
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
}

namespace error
{
	static const char* const invalid_extension = "Invalid file extension: ";
	static const char* const cannot_open_file = "Cannot open config file: ";
}

enum ParserState { OUTSIDE_BLOCK, IN_SERVER, IN_LOCATION };

#endif
