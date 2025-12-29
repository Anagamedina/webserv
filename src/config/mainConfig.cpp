#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include <dirent.h>

#include "../common/constants.hpp"
#include "ConfigParser.hpp"
#include "ConfigException.hpp"


/**
 * returns 0 on success, -1 on error
 * F_OK: check for existence
 * R_OK: check for read permission
*/
bool fileExists(const std::string& path)
{
	return (access(path.c_str(), F_OK | R_OK) == 0);
}

int main(int argc, char* argv[])
{
	try
	{
		std::string configPath;

		if (argc == 1)
		{
			configPath = constants::DEFAULT_CONFIG_PATH;
		}
		else if (argc == 2)
		{
			configPath = argv[1];
		}
		else
		{
			std::cerr << "Usage: ./webserver [config_file.conf]" << std::endl;
			return 1;
		}

		if (!fileExists(configPath))
		{
			std::cerr << "Error: Config file: '" << configPath << "'\nPlease ensure:\n\t1. The file exists\n\t2. You have read permissions\n\t3. You are running from project root: ./webserver";
			return 1;
		}

		// Parse configuration
		const ConfigParser parser(configPath);
		parser.parse();

		// Get parsed servers
		const std::vector<ServerConfig>& servers = parser.getServers();
		std::cout << "✓ Successfully loaded " << servers.size() << " server(s)"
			<< std::endl;

		// Debug: print server
		/*
		for (size_t i = 0; i < servers.size(); ++i)
		{
			std::cout << "\n--- Server " << (i + 1) << " ---" << std::endl;
			std::cout << servers[i];
		}
	*/
	}
	catch (const ConfigException& e)
	{
		std::cerr << "Configuration error: " << e.what() << std::endl;
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
