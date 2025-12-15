#include "ConfigParser.hpp"

#include <fstream>
#include <iostream>

bool ConfigParser::validateExtensionAndPermissionsFile(const std::string& file)
{
	if (file.size() < 5 || file.substr(file.size() - 5) != ".conf")
	{
		//	throw exception
		return false;
	}
	//	validate exists
	std::ifstream ifs(file.c_str());
	if (!ifs.is_open())
	{
		//	throw config exception
		std::cout << "Cannot open config file: " + file;
		return false;
	}
	ifs.close();
	return true;
}

/*
bool ConfigParser::checkIfFileHasValidContent(const std::string& file)
{
}

void ConfigParser::extractBlocksOfEachServer(const std::string& file)
{
}

ConfigParser::ConfigParser()
{
}
*/

ConfigParser::ConfigParser(const std::string& configFile)
{
	configFile_ = configFile;
	serversCount_ = 0;
	// this->servers_ = {};
}

/*
ConfigParser::ConfigParser(const ConfigParser& other)
{
}

ConfigParser::~ConfigParser()
{
}
*/
