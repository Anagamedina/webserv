#include "ConfigParser.hpp"
#include "LocationConfig.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "../common/namespaces.hpp"
#include "ConfigException.hpp"
#include "ConfigUtils.hpp"

ConfigParser::ConfigParser() : servers_count_(0)
{
}

ConfigParser::ConfigParser(const std::string& configFile)
	: config_file_path_(configFile), servers_count_(0U)
{
}

ConfigParser::~ConfigParser()
{
}

//	============= PRIVATE CONSTRUCTORS ===============

ConfigParser::ConfigParser(const ConfigParser& other)
	: config_file_path_(other.config_file_path_),
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

std::string& ConfigParser::getConfigFilePath() { return config_file_path_; }

size_t ConfigParser::getServerCount() const { return servers_count_; }

/**
 * main function of parsing
 *
 */
void ConfigParser::parse()
{
	if (!ValidateFileExtension())
	{
		throw ConfigException(config::errors::invalid_extension +
			config_file_path_);
	}
	else
	{
		std::cout << "VALID FILE EXTENSION: ✅\n";
	}
	if (!ValidateFilePermissions())
	{
		throw ConfigException(
			config::errors::cannot_open_file + config_file_path_);
	}
	else
	{
		std::cout << "VALID FILE PERMISSIONS: ✅\n";
	}

	clean_file_str_ = CleanFileConfig();
	config::utils::exportContentToLogFile(clean_file_str_, config::paths::log_file_config);

	// TODO: need to fix error order of brackets: '} {' should be error but now is
	if (!ValidateCurlyBrackets())
	{
		throw ConfigException("Invalid number of curly brackets " +
			config_file_path_);
	}
	else
	{
		std::cout << "VALID CURLY BRACKETS PAIRS: ✅\n";
	}

	extractServerBlocks();
	parseServers();
}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
	return servers_;
}

/**
 * manage if config_file_path_:
 * has valid size of length
 * has extension '.conf'
 * @return true or false
 */
bool ConfigParser::ValidateFileExtension() const
{
	if (config_file_path_.size() < 5 ||
		config_file_path_.substr(config_file_path_.size() - 5) !=
		config::paths::extension_file)
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
std::string ConfigParser::CleanFileConfig() const
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
		config::utils::removeComments(line);
		line = config::utils::trimLine(line);
		line = config::utils::normalizeSpaces(line);
		if (line.empty())
			continue;
		// logBuffer << "|" << lineNumber << "|" << line << "\n";
		logBuffer << line << "\n";
	}
	ifs.close();
	return logBuffer.str();
}

/**
 * Usar un contador dinámico (incrementa con {, decrementa con }) que nunca baje
de 0
 * Detectar { y }solo cuando son estructurales (inicio/fin de bloque, no dentro
de valores)
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
 * la idea es que dependiendo de que estado se encuentre se actualize el enum,
 * asi saber cuando esta en un bloque de server o location o fuera de bloque
 *
 * extract all blocks 'server'
 * the function extractServerBlock() search all the occurrences to fill the
 * vector raw_server_block_
 */
void ConfigParser::extractServerBlocks()
{
	if (clean_file_str_.empty())
		return;

	extractRawBlocks(clean_file_str_, config::section::server);
}

/**
 * Extracts all server { } blocks from the config content.
 * Handles nested braces within location blocks.
 * @param content The entire config file content
 * @param typeOfExtraction
 */
void ConfigParser::extractRawBlocks(const std::string& content, const std::string& typeOfExtraction)
{
	size_t currentPos = 0;
	size_t countServers = 1;

	while ((currentPos = content.find(typeOfExtraction, currentPos)) != std::string::npos)
	{
		size_t braceStart = content.find('{', currentPos);
		if (braceStart == std::string::npos)
			break;

		// Find matching closing brace
		int countBrackets = 1;
		size_t braceEnd = braceStart + 1;

		while (braceEnd < content.size() - 1 && countBrackets > 0)
		{
			if (content[braceEnd] == '\n')
				braceEnd++;
			// std::cout << "inside loop position [" << braceEnd << "]";
			// std::cout << "\ncontent: [" << content[braceEnd] << "]\n" <<
				// std::endl;
			if (content[braceEnd] == '{')
			{
				countBrackets++;
			}
			else if (content[braceEnd] == '}')
			{
				countBrackets--;
			}
			braceEnd++;
		}

		// Extract the complete server block
		std::string getBlock = content.substr(currentPos, braceEnd - currentPos);

		std::stringstream ss;
		ss << config::paths::log_file_block << "_" << countServers;
		config::utils::exportContentToLogFile(getBlock, ss.str());

		raw_server_blocks_.push_back(getBlock);
		currentPos = braceEnd;
		++countServers;
	}

	servers_count_ = raw_server_blocks_.size();
}

void ConfigParser::parseServers()
{
	for (size_t i = 0; i < raw_server_blocks_.size(); ++i)
	{
		ServerConfig server = parseServer(raw_server_blocks_[i]);
		servers_.push_back(server);
		// std::cout << "Parsing Block " << i + 1 << " [OK]\n";
	}
	std::cout << servers_.at(0);
}

ServerConfig ConfigParser::parseServer(const std::string& blockContent)
{
	ServerConfig server;
	std::stringstream ss(blockContent);
	std::string line;

	while (getline(ss, line))
	{
		int indexTokens = 0;
		line = config::utils::trimLine(line);

		// std::cout << colors::blue << "currentline: [" << line << "]\n"
			// << colors::reset;
		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		// 2. Tokenization
		std::vector<std::string> tokens = config::utils::split(line, ' ');
		if (tokens.empty())
			continue;

		// std::cout << "tokens in line:\n";
		/**
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			std::cout << "token[" << i << "]: |" << colors::yellow << tokens.
				at(i)
				<< colors::reset << "|\n";
		}
		*/

		const std::string& directive = tokens[indexTokens];

		//	LISTEN
		// std::cout << "current directive: [" << directive << "]" << std::endl;
		if (directive == config::section::listen)
		{

			std::string value = tokens[indexTokens + 1];

			// find ':'
			size_t pos = value.find(':');

			if (pos != std::string::npos)
			{
				// Case: IP:PORT (127.0.0.1:8080)
				std::string host = value.substr(0, pos);
				std::string portStr = value.substr(pos + 1);

				server.setHost(host);
				server.setPort(config::utils::stringToInt(portStr));
			}
			else
			{
				// Caso: PORT (8080) or only HOST (localhost)
				// Simple heurística: Si tiene digitos es puerto, sino host
				if (value.find_first_not_of("0123456789") == std::string::npos)
				{
					server.setPort(config::utils::stringToInt(value));
				}
				else
				{
					server.setHost(value);
					server.setPort(80); // Puerto default si solo dan host
				}
			}
		}
		else if (directive == config::section::server_name)
		{
			server.setServerName(config::utils::removeSemicolon(tokens[1]));
		}
		else if (directive == config::section::root)
		{
			server.setRoot(config::utils::removeSemicolon(tokens[1]));
		}
		else if (directive == config::section::index)
		{
			server.setIndex(config::utils::removeSemicolon(tokens[1]));
		}
		else if (directive == config::section::client_max_body_size)
		{
			server.setMaxBodySize(
				config::utils::stringToInt(tokens[1].c_str()));
		}
		else if (directive == config::section::error_page)
		{
			/**
			Lógica especial para múltiples códigos de error
			error_page 404 500 /error.html;
			error_page 404 /404.html;
			error_page 500 502 503 504 /50x.html;
			*/
			if (tokens.size() >= 3)
			{
				std::string path =
					config::utils::removeSemicolon(tokens.back());
				for (size_t i = 1; i < tokens.size() - 1; ++i)
				{
					server.addErrorPage(
						config::utils::stringToInt(tokens[i].c_str()),
						path);
				}
			}
		}
		else if (directive == config::section::location)
		{
			std::string locationPath = tokens[1];
			LocationConfig loc;
			loc.setPath(locationPath);

			while (std::getline(ss, line))
			{
				config::utils::removeComments(line);
				line = config::utils::trimLine(line);
				if (!line.empty())
				{
					std::vector<std::string> locTokens = config::utils::split(
						line, ' ');
					if (locTokens.empty())
					{
						continue;
					}
					if (locTokens[0] == "}")
					{
						break; // End of location block
					}
					if (locTokens[0] == config::section::root)
					{
						loc.setRoot(
							config::utils::removeSemicolon(locTokens[1]));
					}
					else if (locTokens[0] == config::section::index)
					{
						for (size_t i = 1; i < locTokens.size(); ++i)
							loc.addIndex(
								config::utils::removeSemicolon(locTokens[i]));
					}
					else if (locTokens[0] == config::section::autoindex)
					{
						std::string val = config::utils::removeSemicolon(
							locTokens[1]);
						loc.setAutoIndex(val == config::section::autoindex_on);
					}
					else if (locTokens[0] == "methods" ||
						locTokens[0] == "allow_methods")
					{
						for (size_t i = 1; i < locTokens.size(); ++i)
							loc.addMethod(
								config::utils::removeSemicolon(locTokens[i]));
					}
					else if (locTokens[0] == config::section::returnStr)
					// redirection
					{
						// simple support: return 301 /url;
						if (locTokens.size() >= 3)
							loc.setRedirect(
								config::utils::removeSemicolon(locTokens[2]));
					}
					else if (locTokens[0] ==
						config::section::upload_store_bonus)
					{
						loc.setUploadStore(
							config::utils::removeSemicolon(locTokens[1]));
					}
				}
			}
			server.addLocation(loc);
		}
		++indexTokens;
	}
	return server;
}
