#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>

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
			//	TODO: create one function to find the default file of 'configuration'
			configPath = "../config/default.conf";
			std::cout << "Using default config: " << configPath << std::endl;
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
			std::cerr << "Error: Config file not found or not readable: " <<
				configPath
				<<
				"\nPlease ensure:\n1. The file exists\n2. You have read permissions\n3. You are running from project root: ./webserver"
				<< std::endl;
		}

		// Parse configuration
		ConfigParser parser(configPath);
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

	return 0;
}
