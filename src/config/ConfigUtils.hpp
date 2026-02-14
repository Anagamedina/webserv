#ifndef WEBSERV_CONFIGUTILS_HPP
#define WEBSERV_CONFIGUTILS_HPP

#include <vector>

#include "common/namespaces.hpp"

namespace config {
namespace utils {

/** @brief Trims leading and trailing whitespace from a line.*/
std::string trimLine(const std::string& line);

/** @brief Removes comments (starting with #) from a line. */
void removeComments(std::string& line);

/** @brief Removes all spaces and tabs from a string.*/
std::string removeSpacesAndTabs(std::string& line);

/** @brief Replaces multiple whitespace characters with a single space.*/
std::string normalizeSpaces(const std::string& line);

/** @brief Checks if a file exists at the given path.*/
bool fileExists(const std::string& path);

/** @brief Splits a string by a delimiter into a vector.*/
std::vector<std::string> split(const std::string& str, char delimiter);

/** @brief Splits a line into tokens by whitespace.*/
std::vector<std::string> tokenize(const std::string& line);

/** @brief Removes the trailing semicolon from a string if present.*/
std::string removeSemicolon(const std::string& str);

/** @brief Converts a string to an integer.*/
int stringToInt(const std::string& str);

/** @brief Appends content to a log file.*/
void exportContentToLogFile(const std::string& fileContent,
                            const std::string& pathToExport);

/** @brief Validates if a path string is well-formed.*/
bool isValidPath(const std::string& path);

/** @brief Parses a size string (e.g., "1k", "1m") into bytes.*/
long parseSize(const std::string& str);

// New validation functions for TDD

/** @brief Validates an IPv4 address string.*/
bool isValidIPv4(const std::string& ip);

/** @brief Validates a hostname string.*/
bool isValidHostname(const std::string& hostname);

/** @brief Validates a host string (IP or hostname).*/
bool isValidHost(const std::string& host);

/** @brief Validates a location path string.*/
bool isValidLocationPath(const std::string& path);

/** @brief Validates an HTTP method string.*/
bool isValidHttpMethod(const std::string& method);

/** @brief Checks and returns a valid root path.*/
std::string checkRootPath(const std::string& path);

/** @brief Ensures the upload store path exists and is a directory.*/
void ensureUploadStorePath(const std::string& path);

}  // namespace utils

}  // namespace config

#endif  // WEBSERV_CONFIGUTILS_HPP
