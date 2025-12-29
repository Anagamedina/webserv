#include "ConfigParser.hpp"
#include <fstream>
#include <iostream>

#include "ConfigException.hpp"

ConfigParser::ConfigParser() : serversCount_()
{
}

ConfigParser::ConfigParser(const std::string& configFile) : configFilePath_(configFile) , serversCount_(0U)
{
}

ConfigParser::~ConfigParser()
{
}

void ConfigParser::parse()
{
	if (validateExtensionAndPermissionsFile() == true)
	{
		std::cout << "\nwe can open the file: {" << configFilePath_ << "}\n";
	}
	else
	{
		std::cout << "\nError open file :(\n";
	}
}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
	return (servers_);
}

size_t ConfigParser::getServerCount() const
{
	return serversCount_;
}

//	============= PRIVATE CONSTRUCTORS ===============

ConfigParser::ConfigParser(const ConfigParser& other) :
	configFilePath_(other.configFilePath_), serversCount_(other.serversCount_),
	rawServerBlocks_(other.rawServerBlocks_)
{
	// servers_ = other.servers_;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		ConfigParser tmp(other);
		std::swap(configFilePath_, tmp.configFilePath_);
		std::swap(serversCount_, tmp.serversCount_);
		// std::swap(servers_, tmp.servers_);
	}
	return *this;
}

/**
 * manage if configFilePath:
 * has valid size of length
 * has extension '.conf'
 * if is posible open the file.
 * @return
 */
bool ConfigParser::validateExtensionAndPermissionsFile() const
{
	if (configFilePath_.size() < 5 || configFilePath_.substr(configFilePath_.size() - 5) != ".conf")
	{
		//	throw exception
		std::cout << "error with filename: {" << configFilePath_.c_str() << "}\n";
		return false;
	}
	std::cout << "file in validate: {" << configFilePath_.c_str() << "}\n";
	std::ifstream ifs(configFilePath_.c_str());
	if (!ifs.is_open())
	{
		//	throw config exception
		std::cout << "Error: Cannot open config file: [" + configFilePath_ << "]";
		return false;
	}
	ifs.close();
	return true;
}

bool ConfigParser::checkIfFileHasValidContent() const
{
	std::ifstream ifsFile(configFilePath_.c_str());
	if (!ifsFile.is_open())
	{
		// throw ConfigException("Cannot open file: [" + configFilePath_ + "]");
		std::cout << "Cannot open file: [" + configFilePath_ + "]";
	}
	std::string line;
	size_t lineNumber = 0;
	while (std::getline(ifsFile, line))
	{
		lineNumber++;
		const size_t commentPosition = line.find('#');
		if (commentPosition != std::string::npos)
		{
			line = line.substr(0, commentPosition);
		}
		line = trimLine(line);
		if (line.empty())
			continue;
		std::cout << "Line {" << lineNumber <<"} -> [" << line << "]";
	}
	ifsFile.close();
	return true;
}

/**
 * remove includes: space, tab, newline and carriage return
 * 
 * @param line The string to trim
 * @return New string without leading/trailing whitespace
 * 
 * Examples:
 *   "  hello  " -> "hello"
 *   "\t\ntest\r\n" -> "test"
 *   "   " -> ""
 */
std::string ConfigParser::trimLine(std::string& line) const
{
	const std::string whitespace = " \t\n\r";

	const size_t start = line.find_first_not_of(whitespace);
	if (start == std::string::npos)
	{
		return "";
	}
	const size_t end = line.find_last_not_of(whitespace);
	
	return line.substr(start, end - start + 1);
}

/**
 * Reads entire content of config file into a string.
 * @return String containing all file content
 * @throws ConfigException if file cannot be opened
 */
std::string ConfigParser::readFileContent() const
{
	std::ifstream file(configFilePath_.c_str());
	
	if (!file.is_open())
	{
		// throw ConfigException("Cannot open config file: " + configFilePath_);
		return "Cannot open config file: ";
	}
	
	// Read entire file using stringstream
	/*
	std::stringstream buffer;
	buffer << file.rdbuf();
	while (buffer <<  )
	{

	}
	file.close();
	return buffer.str();
	*/
	return "";
}

/**
 * Extracts all server { } blocks from the config content.
 * Handles nested braces within location blocks.
 * @param content The entire config file content
 */
void ConfigParser::extractServerblocks(const std::string& content)
{
	size_t pos = 0;
	
	while ((pos = content.find("server", pos)) != std::string::npos)
	{
		// Find opening brace
		size_t braceStart = content.find('{', pos);
		if (braceStart == std::string::npos)
			break;
		
		// Find matching closing brace (handle nested braces)
		int braceCount = 1;
		size_t braceEnd = braceStart + 1;
		
		while (braceEnd < content.size() && braceCount > 0)
		{
			if (content[braceEnd] == '{')
				braceCount++;
			else if (content[braceEnd] == '}')
				braceCount--;
			braceEnd++;
		}
		
		// Extract the complete server block
		std::string block = content.substr(pos, braceEnd - pos);
		rawServerBlocks_.push_back(block);
		
		pos = braceEnd;
	}
	
	serversCount_ = rawServerBlocks_.size();
}

void ConfigParser::parserServerBlocks() const
{
	// TODO: Implement when ServerConfig is ready
	// For now, just count them
	std::cout << "Found " << serversCount_ << " server block(s)" << std::endl;
}
