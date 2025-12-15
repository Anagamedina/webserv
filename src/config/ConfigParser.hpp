#ifndef WEBSERV_CONFIGPARSER_HPP
#define WEBSERV_CONFIGPARSER_HPP

#include <map>
#include <string>
#include <vector>

class ServerConfig;

class ConfigParser {

    private:
        std::string configFile_;
		unsigned int serversCount_;
        std::vector<ServerConfig> servers_;
        std::vector<std::string> rawServerBlocks_;

		bool validateExtensionAndPermissionsFile(const std::string& file);
		bool checkIfFileHasValidContent(const std::string& file);
		void extractBlocksOfEachServer(const std::string& file);
		void parse();

    public:
        ConfigParser();
        ConfigParser(const std::string& configFile);
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);

        ~ConfigParser();
};

//ostream

#endif //WEBSERV_CONFIGPARSER_HPP