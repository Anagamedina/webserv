#include "ConfigParser.hpp"
#include "LocationConfig.hpp"

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
 * main function of parsing
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
			" in CleanFileConfig()");
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
	for (size_t Index = 0; Index < clean_file_str_.size(); ++Index)
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

/**
 * if line start wirh '#' remove line
 * @param line
 */
void ConfigParser::RemoveComments(std::string& line)
{
	size_t commentPosition = line.find('#');
	if (commentPosition != std::string::npos)
	{
		line = line.substr(0, commentPosition);
	}
}

/**
 * AUX FUNCTION TO DEBUG
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
		std::cerr << "Warning: Could not open/create pretty log file: ";
		return;
	}

	logFile << "=== Pretty print of configuration file ===\n";
	logFile << "File: " << config_file_path_ << "\n";
	logFile << "Generated: " << __DATE__ << " " << __TIME__ << "\n";
	logFile << "----------------------------------------\n\n";

	std::string line;
	size_t lineNum = 0;
	while (std::getline(ifs, line))
	{
		++lineNum;
		RemoveComments(line);
		line = TrimLine(line);
		if (line.empty())
			continue;
		logFile << lineNum << "|" << line << "\n";
	}
	ifs.close();
}

/**
 * remove includes: space, tab, newline and carriage return
 * @param line The string to trim
 * @return New string without leading/trailing whitespace
 * 
 *   "  hello  " -> "hello"
 *   "\t\ntest\r\n" -> "test"
 */
std::string ConfigParser::TrimLine(const std::string& line)
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
 * extract all blocks 'server'
 * the function extractServerBlock() search all the occurrences to fill the vector
 * raw_server_block_
 */
void ConfigParser::MachineStatesOfConfigFile()
{
	if (clean_file_str_.empty())
		return ;

	extractServerBlock(clean_file_str_, "server");
}

/**
 * Extracts all server { } blocks from the config content.
 * Handles nested braces within location blocks.
 * @param content The entire config file content
 * @param typeOfExtraction
 */
void ConfigParser::extractServerBlock(const std::string& content,
									const std::string& typeOfExtraction)
{
	size_t currentPos = 0;
	while ((currentPos = content.find(typeOfExtraction, currentPos)) !=
		std::string::npos)
	{
		size_t braceStart = content.find('{', currentPos);
		if (braceStart == std::string::npos)
			break;

		// Find matching closing brace
		int braceCount = 1;
		size_t braceEnd = braceStart + 1;

		while (braceEnd < content.size() && braceCount > 0)
		{
			if (content[braceEnd] == '{')
			{
				braceCount++;
			}
			else if (content[braceEnd] == '}')
			{
				braceCount--;
			}
			braceEnd++;
		}

		// Extract the complete server block
		std::string getBlock = content.
			substr(currentPos, braceEnd - currentPos);
		raw_server_blocks_.push_back(getBlock);
		currentPos = braceEnd;
	}

	servers_count_ = raw_server_blocks_.size();
}

void ConfigParser::parserServerBlocks() const
{
	for (size_t i = 0; i < raw_server_blocks_.size(); ++i)
	{
		// Llamamos a nuestra función de parseo que retorna un ServerConfig
		// ServerConfig server = parseServerBlock(raw_server_blocks_[i]);
		// servers_.push_back(server); // Asumimos que ConfigParser es non-const o servers_ es mutable
		// Como el metodo es const, no podemos modificar servers_. 
		// Deberíamos quitar el const de parserServerBlocks o hacerlo en parse().
		// Por ahora solo imprimimos para verificar.
		std::cout << "Parsing Block " << i + 1 << "...\n";
		// parseServerBlock(raw_server_blocks_[i]); 
	}
}

ServerConfig ConfigParser::parseServerBlock(const std::string& block)
{
	ServerConfig serverConfig;
	LocationConfig currentLocation;
	config::ParserState state = config::IN_SERVER;
	
	std::stringstream ss(block);
	std::string line;

	while (std::getline(ss, line))
	{
		RemoveComments(line);
		line = TrimLine(line);
		if (line.empty()) continue;

		std::vector<std::string> tokens;
		std::stringstream ss_line(line);
		std::string token;
		while (ss_line >> token) tokens.push_back(token);

		if (state == config::IN_SERVER)
		{
			if (tokens[0] == "server" && tokens[1] == "{") continue; // Inicio de bloque
			if (tokens[0] == "}") break; // Fin de server

			if (tokens[0] == "listen")
			{
				// serverConfig.setPort(std::atoi(tokens[1].c_str()));
				std::cout << "  Configured Port: " << tokens[1] << "\n";
			}
			else if (tokens[0] == "host")
			{
				// serverConfig.setHost(tokens[1]);
				std::cout << "  Configured Host: " << tokens[1] << "\n";
			}
			else if (tokens[0] == "location")
			{
				state = config::IN_LOCATION;
				// currentLocation = LocationConfig();
				// currentLocation.setPath(tokens[1]);
				std::cout << "  >> Entering Location: " << tokens[1] << "\n";
			}
		}
		else if (state == config::IN_LOCATION)
		{
			if (tokens[0] == "}")
			{
				state = config::IN_SERVER;
				// serverConfig.addLocation(currentLocation);
				std::cout << "  << Exiting Location\n";
				continue;
			}
			
			if (tokens[0] == "root")
			{
				// currentLocation.setRoot(tokens[1]);
				std::cout << "    Location Root: " << tokens[1] << "\n";
			}
			else if (tokens[0] == "methods") // example
			{
				std::cout << "    Location Methods: " << tokens[1] << "\n";
			}
		}
	}
	return serverConfig;
}
