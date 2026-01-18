#ifndef WEBSERV_CONFIGPARSER_HPP
#define WEBSERV_CONFIGPARSER_HPP

#include <string>
#include <vector>

#include "ServerConfig.hpp"

class ConfigParser
{
public:
	ConfigParser();
	explicit ConfigParser(const std::string& configFile);
	~ConfigParser();

	//	Getters and Setters
	std::string& getConfigFilePath();
	size_t getServerCount() const;

	void parse();

	const std::vector<ServerConfig>& getServers() const;

private:
	std::string config_file_path_;
	std::string clean_file_str_;
	size_t servers_count_;
	std::vector<std::string> raw_server_blocks_;
	std::vector<ServerConfig> servers_;

	//	constructors of copy and operator
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);

	//	different validations.
	bool ValidateFileExtension() const;
	bool ValidateFilePermissions() const;

	std::string CleanFileConfig();
	bool ValidateCurlyBrackets() const;

	//	auxiliar member function
	void RemoveComments(std::string& line) const;
	void DebugConfigLog() const;
	std::string TrimLine(const std::string& line) const;
	std::string ReadFileContent() const;
	std::string RemoveSpacesAndTabs(std::string& line);
	std::string NormalizeSpaces(const std::string& line);

	void MachineStatesOfConfigFile();
	void extractServerBlocks(const std::string& content);
	void parserServerBlocks() const;
	/**
	ServerConfig parseServerBlock(const std::string& block);
	LocationConfig parseLocationBlock(const std::string& block);
	*/
};

//ostream

#endif //WEBSERV_CONFIGPARSER_HPP
