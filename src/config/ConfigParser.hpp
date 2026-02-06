#ifndef WEBSERV_CONFIGPARSER_HPP
#define WEBSERV_CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <iterator>

#include "ServerConfig.hpp"

class ConfigParser
{
public:
	ConfigParser();
	explicit ConfigParser(const std::string& configFile);
	~ConfigParser();
	//	Getters
	const std::string& getConfigFilePath() const;
	size_t getServerCount() const;
	const std::vector<ServerConfig>& getServers() const;
	// Setters

	void parse();

private:
	std::string config_file_path_;
	std::string clean_file_str_;
	size_t servers_count_;
	std::vector<std::string> raw_server_blocks_;
	std::vector<ServerConfig> servers_;

	//	constructors of copy and operator
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);

	//	validations
	bool validateFileExtension() const;
	bool validateFilePermissions() const;
	bool validateBalancedBrackets() const;

	std::string preprocessConfigFile() const;

	void loadServerBlocks();
	void splitContentIntoServerBlocks(const std::string& content,
						const std::string& typeOfExtraction);

	void parseAllServerBlocks();

	//	TODO: move to serverconfig like function()
	ServerConfig parseSingleServerBlock(const std::string& blockContent);

	// LocationConfig parseLocationBlock(const std::string& block);
};

// ostream

#endif // WEBSERV_CONFIGPARSER_HPP
