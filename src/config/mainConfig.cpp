#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include <dirent.h>

#include "../common/namespaces.hpp"
#include "ConfigParser.hpp"
#include "ConfigException.hpp"

/**
 * returns 0 on success, -1 on msg_errors
 * F_OK: check for existence
 * R_OK: check for read permission
*/
bool fileExists(const std::string& path)
{
	return (access(path.c_str(), F_OK | R_OK) == 0);
}

int main(int argc, char* argv[])
{
	const std::string configPath = (argc > 1)
										? argv[1]
										: config::paths::default_config_path;

	if (!fileExists(configPath))
	{
		std::cerr << "Error: Config file: '" << configPath <<
			"'\nPlease ensure:\n\t1. The file exists\n\t2. You have read permissions\n\t3. You are running from project root: ./webserver";
		return 1;
	}
	try
	{
		ConfigParser parser(configPath);
		std::cout << "Config file path: [" << parser.getConfigFilePath() << "]\n";
		parser.parse();

		/**
		// // Get parsed servers
		// const std::vector<ServerConfig>& servers = parser.getServers();
		// std::cout << "✓ Successfully loaded " << servers.size() << " server(s)"
		// 	<< std::endl;

		// Debug: print server
		for (size_t i = 0; i < servers.size(); ++i)
		{
			std::cout << "\n--- Server " << (i + 1) << " ---" << std::endl;
			std::cout << servers[i];
		}
	*/
	}
	catch (const ConfigException& e)
	{
		std::cerr << "Configuration msg_errors: " << e.what() << std::endl;
		return 1;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	std::flush(std::cout);
	return 0;
}
