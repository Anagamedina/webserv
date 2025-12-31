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

	bool validateFilePath() const;
	bool validateFilePermissions() const;
	bool checkIfFileHasValidContent() const;
	std::string trimLine(std::string& line) ;
	std::string readFileContent() const;

	void extractServerblocks(const std::string& content);
	void parserServerBlocks() const;
	/*
	ServerConfig parseServerBlock(const std::string& block);
	LocationConfig parseLocationBlock(const std::string& block);
	*/
};

//ostream

#endif //WEBSERV_CONFIGPARSER_HPP
