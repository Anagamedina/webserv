#include "config/ConfigException.hpp"
#include "config/ConfigParser.hpp"
#include "network/ServerManager.hpp"

#include <exception>
#include <iostream>
#include <signal.h>

int main(int argc, char *argv[]) {
	std::string config_path = "configs/default.conf";
	if (argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
		return 1;
	}
	if (argc == 2) {
		config_path = argv[1];
	}

	std::cout << "Starting Webserv..." << std::endl;
	std::cout << "Loading config: " << config_path << std::endl;

	// Ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	try {
		ConfigParser parser(config_path);
		ServerConfig config = parser.parse();

		ServerManager server(config);
		server.run();

	} catch (const ConfigException& e) {
		std::cerr << "Configuration Error: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Fatal Runtime Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
