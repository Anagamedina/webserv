#ifndef WEBSERV_CONSTANTS_HPP
#define WEBSERV_CONSTANTS_HPP
#include <string>


namespace constants
{
	static const std::string DEFAULT_CONFIG_PATH = "../config/default.conf";
	// static const std::string DEFAULT_CONFIG_PATH = "../config/examples/nginx.conf";
	static const std::string LOG_FILE_CLEAN= "../config/logs/config-clean.log";
	static const std::string EXTENSION_FILE = ".conf";
}

namespace ansi
{
	// Colores de texto (foreground)
	static const char* const RESET = "\033[0m";
	static const char* const BOLD = "\033[1m";
	static const char* const UNDERLINE = "\033[4m";

	static const char* const BLACK = "\033[30m";
	static const char* const RED = "\033[31m";
	static const char* const GREEN = "\033[32m";
	static const char* const YELLOW = "\033[33m";
	static const char* const BLUE = "\033[34m";
	static const char* const MAGENTA = "\033[35m";
	static const char* const CYAN = "\033[36m";
	static const char* const WHITE = "\033[37m";
}

#endif //WEBSERV_CONSTANTS_HPP
