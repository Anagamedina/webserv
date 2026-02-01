#ifndef WEBSERV_NAMESPACES_HPP
#define WEBSERV_NAMESPACES_HPP
#include <string>

namespace colors
{
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
}

namespace config
{
	namespace paths
	{
		// static const std::string default_config_path = "../config/examples/nginx.conf";
		static const std::string default_config_path = "../config/default.conf";
		static const std::string log_file_config = "../config/logs/config-clean.log";
		static const std::string log_file_server= "../config/logs/server.log";
		static const std::string log_file_block= "../config/logs/block.log";
		static const std::string extension_file = ".conf";
	}

	namespace errors
	{
		static const char* const invalid_extension = "Invalid file extension: ";
		static const char* const cannot_open_file = "Cannot open config file: ";
	}

	namespace section
	{
		static const std::string server = "server";
		static const std::string listen = "listen";
		static const std::string server_name = "server_name";
		static const std::string client_max_body_size = "client_max_body_size";
		static const std::string location = "location";
		static const std::string error_page= "error_page";
		static const std::string root = "root";
		static const std::string index = "index";
		static const std::string autoindex = "autoindex";
		static const std::string autoindex_on = "on";
		static const std::string autoindex_off = "off";
		static const std::string returnStr = "return";
		static const std::string upload_store_bonus= "upload_store";

	}

	enum ParserState
	{
		OUTSIDE_BLOCK,
		IN_SERVER,
		IN_LOCATION
	};

	namespace debug
	{
		void debugConfigLog(const std::string& config_file_path);

	}

}

#endif
