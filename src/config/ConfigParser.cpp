#include "ConfigParser.hpp"
#include "ConfigException.hpp"
#include <cstdlib> // atoi
#include <fstream>
#include <iostream>
#include <sstream>

ConfigParser::ConfigParser(const std::string &config_path)
    : config_path_(config_path), current_pos_(0) {}

ConfigParser::~ConfigParser() {}

ServerConfig ConfigParser::parse() {
  loadFile();
  ServerConfig config;

  while (current_pos_ < file_content_.length()) {
    skipWhitespace();
    if (current_pos_ >= file_content_.length())
      break;

    std::string token = parseToken();
    if (token.empty())
      break;

    if (token == "server") {
      parseServerBlock(config);
    } else {
      throw ConfigException("Unexpected token at top level: " + token);
    }
  }

  return config;
}

void ConfigParser::loadFile() {
  std::ifstream file(config_path_.c_str());
  if (!file.is_open()) {
    throw ConfigException("Could not open config file: " + config_path_);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file_content_ = buffer.str();
}

void ConfigParser::skipWhitespace() {
  while (current_pos_ < file_content_.length()) {
    char c = file_content_[current_pos_];
    if (isspace(c)) {
      current_pos_++;
    } else if (c == '#') {
      skipComment();
    } else {
      break;
    }
  }
}

void ConfigParser::skipComment() {
  while (current_pos_ < file_content_.length() &&
         file_content_[current_pos_] != '\n') {
    current_pos_++;
  }
}

std::string ConfigParser::parseToken() {
  skipWhitespace();
  if (current_pos_ >= file_content_.length())
    return "";

  // Handle special characters that act as tokens themselves
  char c = file_content_[current_pos_];
  if (c == '{' || c == '}' || c == ';') {
    current_pos_++;
    return std::string(1, c);
  }

  size_t start = current_pos_;
  while (current_pos_ < file_content_.length()) {
    c = file_content_[current_pos_];
    if (isspace(c) || c == '{' || c == '}' || c == ';' || c == '#') {
      break;
    }
    current_pos_++;
  }
  return file_content_.substr(start, current_pos_ - start);
}

void ConfigParser::expect(const std::string &expected) {
  std::string token = parseToken();
  if (token != expected) {
    throw ConfigException("Expected '" + expected + "', found '" + token + "'");
  }
}

void ConfigParser::parseServerBlock(ServerConfig &config) {
  expect("{");

  ServerBlock server;
  while (true) {
    std::string token = parseToken();
    if (token == "}")
      break;
    if (token == "")
      throw ConfigException("Unexpected end of file inside server block");

    if (token == "listen") {
      server.port = parsePort(parseToken());
      expect(";");
    } else if (token == "host") {
      server.host = parseToken();
      expect(";");
    } else if (token == "server_name") {
      while (true) {
        std::string name = parseToken();
        if (name == ";")
          break;
        if (name == "{" || name == "}")
          throw ConfigException("Unexpected token in server_name");
        server.server_names.push_back(name);
      }
    } else if (token == "root") {
      server.root = parseToken();
      expect(";");
    } else if (token == "index") {
      server.index = parseToken();
      expect(";");
    } else if (token == "error_page") {
      int code = atoi(parseToken().c_str());
      std::string path = parseToken();
      server.error_pages[code] = path;
      expect(";");
    } else if (token == "client_max_body_size") {
      server.client_max_body_size = parseSize(parseToken());
      expect(";");
    } else if (token == "location") {
      std::string path = parseToken();
      parseLocationBlock(server, path);
    } else {
      // Ignore unknown directives or throw error
      throw ConfigException("Unknown directive in server block: " + token);
    }
  }
  config.servers.push_back(server);
}

void ConfigParser::parseLocationBlock(ServerBlock &server,
                                      const std::string &path) {
  expect("{");
  LocationConfig loc;
  loc.path = path;

  while (true) {
    std::string token = parseToken();
    if (token == "}")
      break;
    if (token == "")
      throw ConfigException("Unexpected end of file inside location block");

    if (token == "root") {
      loc.root = parseToken();
      expect(";");
    } else if (token == "index") {
      loc.index = parseToken();
      expect(";");
    } else if (token == "autoindex") {
      std::string val = parseToken();
      if (val == "on")
        loc.autoindex = true;
      else if (val == "off")
        loc.autoindex = false;
      else
        throw ConfigException("Invalid autoindex value: " + val);
      expect(";");
    } else if (token == "allow_methods") {
      while (true) {
        std::string method = parseToken();
        if (method == ";")
          break;
        loc.accepted_methods.insert(method);
      }
    } else if (token == "return") { // Redirection
      loc.redirect_url = parseToken();
      expect(";");
    } else if (token == "upload_enabled") {
      std::string val = parseToken();
      if (val == "on")
        loc.upload_enabled = true;
      else if (val == "off")
        loc.upload_enabled = false;
      else
        throw ConfigException("Invalid upload_enabled value: " + val);
      expect(";");
    } else if (token == "upload_path") {
      loc.upload_path = parseToken();
      expect(";");
    } else if (token == "cgi_handler") {
      std::string ext = parseToken();
      std::string interpreter = parseToken();
      loc.cgi_handlers[ext] = interpreter;
      expect(";");
    } else if (token == "client_max_body_size") {
      std::string size_str = parseToken();
      loc.client_max_body_size = parseSize(size_str);
      expect(";");
    } else {
      throw ConfigException("Unknown directive in location block: " + token);
    }
  }
  server.locations.push_back(loc);
}

int ConfigParser::parsePort(const std::string &token) {
  return atoi(token.c_str());
}

size_t ConfigParser::parseSize(const std::string &token) {
  // Check for 'M', 'K', 'G' suffixes
  size_t multiplier = 1;
  std::string num_part = token;

  if (!token.empty()) {
    char suffix = token[token.length() - 1];
    if (suffix == 'M' || suffix == 'm') {
      multiplier = 1024 * 1024;
      num_part = token.substr(0, token.length() - 1);
    } else if (suffix == 'K' || suffix == 'k') {
      multiplier = 1024;
      num_part = token.substr(0, token.length() - 1);
    }
  }
  return atoi(num_part.c_str()) * multiplier;
}
