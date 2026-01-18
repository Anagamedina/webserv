#include "ConfigParser.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ConfigException.hpp"
#include "../common/namespaces.hpp"

ConfigParser::ConfigParser() : servers_count_(0)
{
}

ConfigParser::ConfigParser(const std::string& configFile) :
	config_file_path_(configFile), servers_count_(0U)
{
}

ConfigParser::~ConfigParser()
{
}

//	============= PRIVATE CONSTRUCTORS ===============

ConfigParser::ConfigParser(const ConfigParser& other) :
	config_file_path_(other.config_file_path_),
	servers_count_(other.servers_count_),
	raw_server_blocks_(other.raw_server_blocks_)
{
	// servers_ = other.servers_;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		ConfigParser tmp(other);
		std::swap(config_file_path_, tmp.config_file_path_);
		std::swap(servers_count_, tmp.servers_count_);
		// std::swap(servers_, tmp.servers_);
	}
	return *this;
}

//	Getters and Setters

std::string& ConfigParser::getConfigFilePath()
{
	return config_file_path_;
}

size_t ConfigParser::getServerCount() const
{
	return servers_count_;
}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
	return servers_;
}

/**
 * todas las validaciones del archivo .conf
 * se gestionan en esta funcion.
 *
 */
void ConfigParser::parse()
{
	if (!ValidateFileExtension())
	{
		throw ConfigException(
			config::errors::invalid_extension + config_file_path_);
	}
	if (!ValidateFilePermissions())
	{
		throw ConfigException(
			config::errors::cannot_open_file + config_file_path_);
	}

	clean_file_str_ = CleanFileConfig();
	std::ofstream log(config::paths::log_file.data());
	log << clean_file_str_;
	log.close();

	// std::cout << "CLEANFILESTR in PARSE:\n" << clean_file_str_.c_str();

	if (!ValidateCurlyBrackets())
	{
		throw ConfigException(
			"Invalid number of curly brackets " + config_file_path_);
	}
	else
	{
		std::cout << "curly brackerts correct :)\n";
	}
}

/**
 * manage if config_file_path_:
 * has valid size of length
 * has extension '.conf'
 * @return true or false
 */
bool ConfigParser::ValidateFileExtension() const
{
	if (config_file_path_.size() < 5 || config_file_path_.substr(
		config_file_path_.size() - 5) != config::paths::extension_file)
	{
		return false;
	}
	return true;
}

bool ConfigParser::ValidateFilePermissions() const
{
	std::ifstream ifs(config_file_path_.c_str());
	if (!ifs.is_open())
		return false;
	ifs.close();
	return true;
}

/**
 * RemoveComments(): if line start with '#' skip line
 * TrimLine(): if line find "\t\n\r" remove character
 * NormalizeSpaces(): replace 'X' spaces for one space ' '
 * iterate through each line of file.
 * @return
 */
std::string ConfigParser::CleanFileConfig()
{
	std::ifstream ifs(config_file_path_.c_str());
	if (!ifs.is_open())
	{
		throw ConfigException(
			config::errors::cannot_open_file + config_file_path_ +
			" in validateBasicContent");
	}

	std::ostringstream logBuffer;
	std::string line;
	// size_t lineNumber = 0;

	while (std::getline(ifs, line))
	{
		// ++lineNumber;
		RemoveComments(line);
		line = TrimLine(line);
		line = NormalizeSpaces(line);
		if (line.empty())
			continue;
		// logBuffer << "|" << lineNumber << "|" << line << "\n";
		logBuffer << line << "\n";
	}
	ifs.close();
	return logBuffer.str();
}

/**
* Usar un contador dinámico (incrementa con {, decrementa con }) que nunca baje de 0
* Detectar { y }solo cuando son estructurales (inicio/fin de bloque, no dentro de valores)
* Manejar casos como:
* server { (abre)
* } (cierra)
* location /path { (abre)
* Cierres prematuros

 * @return true solo si:
 * Contador nunca negativo durante la lectura
 * Contador == 0 al final
 * Mejorar mensajes de error (línea + descripción)
 * Opcional: devolver también el nivel máximo de anidamiento o lista de errores
 */
bool ConfigParser::ValidateCurlyBrackets() const
{
	int countBrackets = 0;
	for (u_int32_t Index = 0; Index < clean_file_str_.size(); ++Index)
	{
		if (clean_file_str_.at(Index) == '{')
		{
			++countBrackets;
		}
		else if (clean_file_str_.at(Index) == '}')
		{
			--countBrackets;
			if (countBrackets < 0)
			{
				return false;
			}
		}
	}
	return countBrackets == 0;
}


//	aux member functions
void ConfigParser::RemoveComments(std::string& line) const
{
	size_t commentPosition = line.find('#');
	if (commentPosition != std::string::npos)
	{
		line = line.substr(0, commentPosition);
	}
}

/**
 * export config file '.log'
 * remove empty lines and comment lines.
 */
void ConfigParser::DebugConfigLog() const
{
	std::ifstream ifs(config_file_path_.c_str());
	if (!ifs.is_open())
	{
		throw ConfigException(
			config::errors::cannot_open_file +
			config_file_path_ +
			" (in generatePrettyConfigLog)"
		);
	}
	std::ofstream logFile(config::paths::log_file.c_str());
	if (!logFile.is_open())
	{
		// Aquí puedes elegir: lanzar excepción o solo warning
		std::cerr << "Warning: Could not open/create pretty log file: ";
		//<< config::paths::log_file_output << "\n";
		// throw ConfigException("Cannot write pretty log");
		return;
	}
	logFile << "=== Pretty print of configuration file ===\n";
	logFile << "File: " << config_file_path_ << "\n";
	logFile << "Generated: " << __DATE__ << " " << __TIME__ << "\n";
	logFile << "----------------------------------------\n\n";

	std::string line;

	// size_t lineNum = 0;
	while (std::getline(ifs, line))
	{
		// ++lineNum;
		RemoveComments(line);
		line = TrimLine(line);
		if (line.empty())
			continue;
		//logFileOutput << lineNum << "|" << line << "\n";
		logFile << line << "\n";
	}
	ifs.close();
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
std::string ConfigParser::TrimLine(const std::string& line) const
{
	const std::string whitespace = "\t\n\r";

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
std::string ConfigParser::ReadFileContent() const
{
	std::ifstream file(config_file_path_.c_str());

	if (!file.is_open())
	{
		throw ConfigException("Cannot open config file: " + config_file_path_);
		// return "Cannot open config file: ";
	}
	// Read entire file using stringstream
	/**
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

struct IsConsecutiveSpace
{
	bool operator()(char a, char b) const { return a == ' ' && b == ' '; }
};

std::string ConfigParser::RemoveSpacesAndTabs(std::string& line)
{
	line.erase(std::unique(line.begin(), line.end(), IsConsecutiveSpace()),
				line.end());
	return line;
}

std::string ConfigParser::NormalizeSpaces(const std::string& line)
{
	std::stringstream ss(line);
	std::string word;
	std::string result;

	while (ss >> word)
	{
		if (!result.empty())
			result += " ";
		result += word;
	}
	return result;
}

/**
 * la idea es que dependiendo de que estado se encuentre se actualize el enum,
 * asi saber cuando esta en un bloque de server o location o fuera de bloque
 *
 */
//	TODO: reorganize this part to understand whar we need to do

/*
void ConfigParser::MachineStatesOfConfigFile()
{
	config::ParserState state = config::OUTSIDE_BLOCK;
	size_t braceCount = 0;
	size_t countLines = 0;

	std::ifstream ifs(config_file_path_.c_str());
	if (!ifs.is_open())
	{
		std::ostringstream oss;
		oss << config::errors::cannot_open_file << config_file_path_ <<
			" in MachineStatesOfConfigFile()";
		throw ConfigException(oss.str());
	}

	std::string line;
	config::ParserState state;

	while (getline(ifs, line))
	{
		++countLines;

		RemoveComments(line);
		line = TrimLine(line);

		if (line.empty())
			continue ;
	}
}
*/

/**
 * Extracts all server { } blocks from the config content.
 * Handles nested braces within location blocks.
 * @param content The entire config file content
 */
void ConfigParser::extractServerBlocks(const std::string& content)
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
		raw_server_blocks_.push_back(block);

		pos = braceEnd;
	}

	servers_count_ = raw_server_blocks_.size();
}

void ConfigParser::parserServerBlocks() const
{
	// TODO: Implement when ServerConfig is ready
	// For now, just count them
	std::cout << "Found " << servers_count_ << " server block(s)" << std::endl;
}
