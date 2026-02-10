#ifndef WEBSERV_CONFIGUTILS_HPP
#define WEBSERV_CONFIGUTILS_HPP

#include <vector>

#include "common/namespaces.hpp"

namespace config {
namespace utils {

std::string trimLine(const std::string& line);
void removeComments(std::string& line);
std::string removeSpacesAndTabs(std::string& line);
std::string normalizeSpaces(const std::string& line);
bool fileExists(const std::string& path);

std::vector<std::string> split(const std::string& str, char delimiter);
std::vector<std::string> tokenize(const std::string& line);
std::string removeSemicolon(const std::string& str);
int stringToInt(const std::string& str);
void exportContentToLogFile(const std::string& fileContent,
                            const std::string& pathToExport);
bool isValidPath(const std::string& path);
long parseSize(const std::string& str);

// New validation functions for TDD
bool isValidIPv4(const std::string& ip);
bool isValidHostname(const std::string& hostname);
bool isValidHost(const std::string& host);
bool isValidLocationPath(const std::string& path);
bool isValidHttpMethod(const std::string& method);
std::string checkRootPath(const std::string& path);
void ensureUploadStorePath(const std::string& path);

}  // namespace utils

}  // namespace config

#endif  // WEBSERV_CONFIGUTILS_HPP
