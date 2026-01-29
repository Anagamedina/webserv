#pragma once

#include "ServerConfig.hpp"
#include <string>
#include <vector>

class ConfigParser {
public:
    ConfigParser(const std::string& config_path);
    ~ConfigParser();

    ServerConfig parse();

private:
    std::string config_path_;
    std::string file_content_;
    size_t current_pos_;

    void loadFile();
    void skipWhitespace();
    void skipComment();
    std::string parseToken();
    void expect(const std::string& token);
    
    // Parsing helpers
    void parseServerBlock(ServerConfig& config);
    void parseLocationBlock(ServerBlock& server, const std::string& path);
    
    // Utils
    int parsePort(const std::string& token);
    size_t parseSize(const std::string& token);
};
