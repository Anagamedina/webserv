#include "ConfigUtils.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "ConfigException.hpp"

namespace config {
namespace utils {
/**
 * remove in line: space, tab, newline and carriage return
 * @param line The string to trim
 * @return New string without leading/trailing whitespace
 *
 *   "  hello  " -> "hello"
 *   "\t\ntest\r\n" -> "test"
 */
std::string trimLine(const std::string& line) {
  const std::string whitespace = "\t\n\r";

  const size_t start = line.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = line.find_last_not_of(whitespace);
  return line.substr(start, end - start + 1);
}

/**
 * if line start wirh '#' remove line
 * @param line
 */
void removeComments(std::string& line) {
  size_t commentPosition = line.find('#');
  if (commentPosition != std::string::npos) {
    line = line.substr(0, commentPosition);
  }
}

struct IsConsecutiveSpace {
  bool operator()(char a, char b) const { return a == ' ' && b == ' '; }
};

std::string removeSpacesAndTabs(std::string& line) {
  line.erase(std::unique(line.begin(), line.end(), IsConsecutiveSpace()),
             line.end());
  return line;
}

std::string normalizeSpaces(const std::string& line) {
  std::stringstream ss(line);
  std::string word;
  std::string result;

  while (ss >> word) {
    if (!result.empty()) result += " ";
    result += word;
  }
  return result;
}

/**
 * returns 0 on success, -1 on msg_errors
 * F_OK: check for existence
 * R_OK: check for read permission
 */
bool fileExists(const std::string& path) {
  return (access(path.c_str(), F_OK | R_OK) == 0);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);

  if (delimiter == ' ') {
    while (tokenStream >> token) tokens.push_back(token);
  } else {
    while (std::getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
    }
  }
  return tokens;
}

std::vector<std::string> tokenize(const std::string& line) {
  std::vector<std::string> tokens;
  std::string currentToken;
  bool inQuotes = false;
  // char quoteChar = 0; // ' or "

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];

    if (inQuotes) {
      if (c == '"') {
        inQuotes = false;
        // quoteChar = 0;
        // Don't add quote to token?
        // Nginx usually strips quotes.
        // tokens.push_back(currentToken);
        // currentToken.clear();
      } else {
        currentToken += c;
      }
    } else {
      if (std::isspace(c)) {
        if (!currentToken.empty()) {
          tokens.push_back(currentToken);
          currentToken.clear();
        }
      } else if (c == '"') {
        inQuotes = true;
        // quoteChar = c;
      } else if (c == ';') {
        /*
        if (!currentToken.empty()) {
          tokens.push_back(currentToken);
          currentToken.clear();
        }*/
        // tokens.push_back(";"); // Keep semicolon as separate token?
        // ConfigParser expects tokens with semicolons attached usually or
        // stripped manually. Current split behavior: "listen 80;" -> "listen",
        // "80;" So we should attach semicolon if it's part of the word, or...
        // Wait, split(' ') keeps "80;" as "80;".
        // But here we are iterating chars.
        // If we hit ';', we end the token.
        // We can append ';' to currentToken before clearing?
        // Or just treat it as a char unless we want to be smart.
        // Let's just treat it as normal char if not space.
        currentToken += c;
      } else if (c == '#') {
        // Comment detected, stop parsing line
        if (!currentToken.empty()) {
          tokens.push_back(currentToken);
        }
        return tokens;
      } else {
        currentToken += c;
      }
    }
  }
  if (!currentToken.empty()) {
    tokens.push_back(currentToken);
  }
  return tokens;
}

/**
 * use find() function to search position of ';'
 * @return substr of str
 */
std::string removeSemicolon(const std::string& str) {
  size_t pos = str.find(';');
  if (pos != std::string::npos) {
    return str.substr(0, pos);
  }
  return str;
}

int stringToInt(const std::string& str) {
  char* end;
  long value = std::strtol(str.c_str(), &end, 10);

  if (*end != '\0' || end == str.c_str()) {
    throw ConfigException(config::errors::invalid_characters);
  }
  if (value > std::numeric_limits<int>::max() ||
      value < std::numeric_limits<int>::min()) {
    throw ConfigException(config::errors::number_out_of_range);
  }

  return static_cast<int>(value);
}

void exportContentToLogFile(const std::string& fileContent,
                            const std::string& pathToExport) {
  std::ofstream log(pathToExport.data());
  log << fileContent;
  log.close();
}

bool isValidPath(const std::string& path) {
  if (path.empty()) return false;

  if (path.find('\0') != std::string::npos ||
      path.find('\n') != std::string::npos ||
      path.find('\r') != std::string::npos)
    return false;

  return true;
}

long parseSize(const std::string& str) {
  if (str.empty()) {
    throw ConfigException("Empty size string");
  }

  char* end;
  long value = std::strtol(str.c_str(), &end, 10);

  if (end == str.c_str()) {
    throw ConfigException(config::errors::invalid_characters + str);
  }

  if (value < 0) {
    throw ConfigException(config::errors::body_size_negative);
  }

  std::string suffix = end;
  if (!suffix.empty()) {
    if (suffix.length() > 1) {
      throw ConfigException(config::errors::invalid_characters + str);
    }

    char s = std::tolower(suffix[0]);
    // Check for overflow before multiplication
    const long MAX_SIZE =
        std::numeric_limits<int>::max();  // 2GB limit (INT_MAX)

    if (s == 'k') {
      if (value > MAX_SIZE / 1024) {
        throw ConfigException(config::errors::body_size_overflow);
      }
      value *= 1024;
    } else if (s == 'm') {
      if (value > MAX_SIZE / (1024 * 1024)) {
        throw ConfigException(config::errors::body_size_overflow);
      }
      value *= 1024 * 1024;
    } else if (s == 'g') {
      if (value > MAX_SIZE / (1024 * 1024 * 1024)) {
        throw ConfigException(config::errors::body_size_overflow);
      }
      value *= 1024 * 1024 * 1024;
    } else {
      throw ConfigException("Invalid size suffix: " + suffix);
    }
  }

  if (value < 0 || value > std::numeric_limits<int>::max()) {
    throw ConfigException(config::errors::body_size_overflow);
  }

  return value;
}

// ============================================================================
// New validation functions for TDD
// ============================================================================

// Validates IPv4 address format (xxx.xxx.xxx.xxx where xxx is 0-255)
bool isValidIPv4(const std::string& ip) {
  if (ip.empty()) {
    return false;
  }

  std::vector<std::string> octets = split(ip, '.');
  if (octets.size() != 4) {
    return false;
  }

  for (size_t i = 0; i < octets.size(); ++i) {
    const std::string& octet = octets[i];

    // Check if empty or has invalid length
    if (octet.empty() || octet.length() > 3) {
      return false;
    }

    // Check all characters are digits
    for (size_t j = 0; j < octet.length(); ++j) {
      if (!std::isdigit(static_cast<unsigned char>(octet[j]))) {
        return false;
      }
    }

    // Convert to int and check range
    int val = 0;
    for (size_t j = 0; j < octet.length(); ++j) {
      val = val * 10 + (octet[j] - '0');
    }

    // Check range 0-255
    if (val > 255) {
      return false;
    }
  }

  return true;
}

// Validates hostname format following RFC 952 and RFC 1123
// - Can contain alphanumeric characters, dots, and hyphens
// - Cannot start or end with hyphen or dot
// - Hyphens cannot be immediately before or after dots
// - No consecutive dots
bool isValidHostname(const std::string& hostname) {
  if (hostname.empty()) {
    return false;
  }

  // Check for invalid starting/ending characters
  if (hostname[0] == '-' || hostname[0] == '.' ||
      hostname[hostname.length() - 1] == '-' ||
      hostname[hostname.length() - 1] == '.') {
    return false;
  }

  // Check each character and for consecutive dots and hyphens near dots
  for (size_t i = 0; i < hostname.length(); ++i) {
    char c = hostname[i];

    // Valid characters: alphanumeric, dot, hyphen
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-') {
      return false;
    }

    // Check for consecutive dots
    if (c == '.' && i + 1 < hostname.length() && hostname[i + 1] == '.') {
      return false;
    }

    // Check for hyphen immediately before a dot (e.g., "example-.com")
    if (c == '-' && i + 1 < hostname.length() && hostname[i + 1] == '.') {
      return false;
    }

    // Check for hyphen immediately after a dot (e.g., "example.-com")
    if (c == '-' && i > 0 && hostname[i - 1] == '.') {
      return false;
    }
  }

  return true;
}

// Validates a host string (either IPv4 or hostname)
bool isValidHost(const std::string& host) {
  if (host.empty()) {
    return false;
  }

  bool looksLikeIPv4 = true;
  int dotCount = 0;
  for (size_t i = 0; i < host.length(); ++i) {
    if (host[i] == '.') {
      dotCount++;
    } else if (!std::isdigit(static_cast<unsigned char>(host[i]))) {
      looksLikeIPv4 = false;
      break;
    }
  }

  if (looksLikeIPv4 && dotCount == 3) {
    return isValidIPv4(host);
  }

  return isValidHostname(host);
}

// Validates location path format:
// - Must start with '/'
// - Cannot be empty
// - No leading/trailing whitespace
// - No double slashes
bool isValidLocationPath(const std::string& path) {
  if (path.empty()) {
    return false;
  }

  // Must start with '/'
  if (path[0] != '/') {
    return false;
  }

  // Check for whitespace
  if (path.find(' ') != std::string::npos ||
      path.find('\t') != std::string::npos) {
    return false;
  }

  // Check for double slashes (except at position 0)
  for (size_t i = 1; i < path.length(); ++i) {
    if (path[i] == '/' && path[i - 1] == '/') {
      return false;
    }
  }

  return true;
}

// Validates HTTP method against whitelist (GET, POST, DELETE only)
bool isValidHttpMethod(const std::string& method) {
  return (method == "GET" || method == "POST" || method == "DELETE");
}

// Checks if a root path exists and is accessible.
// Returns empty string if OK, warning message if not.
// Following NGINX behavior: warns but does not fail at startup.
std::string checkRootPath(const std::string& path) {
  if (path.empty()) {
    return config::errors::root_path_warning + ": path is empty";
  }

  // Use stat to check if path exists and is a directory
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    return config::errors::root_path_warning + ": " + path + " does not exist";
  }

  if (!(info.st_mode & S_IFDIR)) {
    return config::errors::root_path_warning + ": " + path +
           " is not a directory";
  }

  return "";
}

// Ensures upload store path exists, creating it if necessary.
// Throws ConfigException if creation fails.
// Following NGINX behavior: creates directory or fails at startup.
void ensureUploadStorePath(const std::string& path) {
  if (path.empty()) {
    throw ConfigException(config::errors::upload_store_creation_failed +
                          ": path is empty");
  }

  struct stat info;
  if (stat(path.c_str(), &info) == 0) {
    if (!(info.st_mode & S_IFDIR)) {
      throw ConfigException(config::errors::upload_store_creation_failed +
                            ": " + path + " exists but is not a directory");
    }
    return;
  }

  // Directory doesn't exist, try to create it
  // Note: mkdir creates only the last component, not parent directories
  // For simplicity, we'll create just the final directory
  // In production, you might want to create parent directories too
  if (mkdir(path.c_str(), 0755) != 0) {
    throw ConfigException(config::errors::upload_store_creation_failed + ": " +
                          path + " (" + std::string(strerror(errno)) + ")");
  }
}
}  // namespace utils

namespace debug {
/**
 * AUX FUNCTION TO DEBUG
 * export config file '.log'
 * remove empty lines and comment lines.
 */
void debugConfigLog(const std::string& config_file_path) {
  std::ifstream ifs(config_file_path.c_str());
  if (!ifs.is_open()) {
    throw ConfigException(config::errors::cannot_open_file + config_file_path +
                          " (in generatePrettyConfigLog)");
  }

  std::ofstream logFile(config::paths::log_file_config.c_str());
  if (!logFile.is_open()) {
    std::cerr << "Warning: Could not open/create pretty log file: ";
    return;
  }

  logFile << "=== Pretty print of configuration file ===\n";
  logFile << "File: " << config_file_path << "\n";
  logFile << "Generated: " << __DATE__ << " " << __TIME__ << "\n";
  logFile << "----------------------------------------\n\n";

  std::string line;
  size_t lineNum = 0;
  while (std::getline(ifs, line)) {
    ++lineNum;
    config::utils::removeComments(line);
    line = config::utils::trimLine(line);
    if (line.empty()) continue;
    logFile << lineNum << "|" << line << "\n";
  }
  ifs.close();
}
}  // namespace debug
}  // namespace config
