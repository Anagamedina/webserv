#ifndef WEBSERV_CONFIGPARSER_HPP
#define WEBSERV_CONFIGPARSER_HPP

#include <string>
#include <vector>

class ServerConfig;
class LocationConfig;

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

	bool validateExtensionAndPermissionsFile() const;
	bool checkIfFileHasValidContent() const ;
	std::string trimLine(std::string& line) const ;
	std::string readFileContent();

	void extractServerblocks(const std::string& content);
	void parserServerBlocks();
	/*
	ServerConfig parseServerBlock(const std::string& block);
	LocationConfig parseLocationBlock(const std::string& block);
	*/
};

//ostream

#endif //WEBSERV_CONFIGPARSER_HPP
