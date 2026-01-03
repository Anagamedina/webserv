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

	void parse();
	std::string& getConfigFilePath();
	const std::vector<ServerConfig>& getServers() const;
	size_t getServerCount() const;

private:
	std::string configFilePath_;
	size_t serversCount_;
	std::vector<std::string> rawServerBlocks_;
	std::vector<ServerConfig> servers_;

	//	constructors of copy and operator
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);

	bool validateFileExtension() const;
	bool validateFilePermissions() const;
	bool validateBasicContent() const;

	void removeComments(std::string& line) const;
	void exportCleanConfigFile(const std::ostringstream& logBuffer) const;
	std::string trimLine(std::string& line) const;
	std::string readFileContent() const;

	void extractServerblocks(const std::string& content);
	void parserServerBlocks() const;
	/**
	ServerConfig parseServerBlock(const std::string& block);
	LocationConfig parseLocationBlock(const std::string& block);
	*/
};

//ostream

#endif //WEBSERV_CONFIGPARSER_HPP
