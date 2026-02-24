#ifndef WEBSERV_CONFIGPARSER_HPP
#define WEBSERV_CONFIGPARSER_HPP

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "ServerConfig.hpp"

/**
 * @brief Parses a nginx-like configuration file and builds ServerConfig
 * objects.
 *
 * Loads, validates and parses .conf file into vector<ServerConfig>.
 * Supports server {}, location {}, basic directives and some bonus features.
 * Main usage:
 *   - ConfigParser parser("webserv.conf");
 *   - parser.parse();
 *   - const servers = parser.getServers();
 */
class ConfigParser {
 public:
  ConfigParser();
  explicit ConfigParser(const std::string& configFile);
  ~ConfigParser();

  //	Getters
  const std::string& getConfigFilePath() const;
  size_t getServerCount() const;
  const std::vector<ServerConfig>& getServers() const;

  void parse();

 private:
  // constructors of copy and operator
  ConfigParser(const ConfigParser& other);
  ConfigParser& operator=(const ConfigParser& other);

  // File validations
  bool validateFileExtension() const;
  bool validateFilePermissions() const;
  bool validateBalancedBrackets() const;

  // Preprocessing and block extraction
  std::string preprocessConfigFile() const;
  void loadServerBlocks();
  void splitContentIntoServerBlocks(const std::string& content,
                                    const std::string& typeOfExtraction);
  // Parsing logic
  void parseAllServerBlocks();
  ServerConfig parseSingleServerBlock(const std::string& blockContent);

  // Directive parsers (server level)
  void parseListen(ServerConfig& server,
                   const std::vector<std::string>& tokens);
  void parseHost(ServerConfig& server, const std::vector<std::string>& tokens);
  void parseServerName(ServerConfig& server,
                       const std::vector<std::string>& tokens);
  void parseRoot(ServerConfig& server, const std::vector<std::string>& tokens);
  void parseIndex(ServerConfig& server, const std::vector<std::string>& tokens);
  void parseMaxSizeBody(ServerConfig& server,
                        const std::vector<std::string>& tokens);
  void parseErrorPage(ServerConfig& server, std::vector<std::string>& tokens);

  // Location & bonus parsers
  void parseLocationBlock(ServerConfig& server, std::stringstream& ss,
                          std::string& line,
                          const std::vector<std::string>& tokens);
  void parseMaxSizeBody(LocationConfig& loc,
                        const std::vector<std::string>& tokens);
  void parseCgi(LocationConfig& loc, const std::vector<std::string>& tokens);
  void parseUploadBonus(LocationConfig& loc,
                        std::vector<std::string>& locTokens);
  void parseReturn(LocationConfig& loc, std::vector<std::string>& locTokens);

  // Validation helpers
  void validateDirectiveLine(const std::string& line) const;
  void checkDuplicateServerConfig() const;

  /** @brief Returns the directory part of the config file path. */
  std::string getConfFileDir() const;

 private:
  std::string config_file_path_;
  std::string clean_file_str_;
  size_t servers_count_;
  std::vector<std::string> raw_server_blocks_;
  std::vector<ServerConfig> servers_;
};

#endif  // WEBSERV_CONFIGPARSER_HPP
