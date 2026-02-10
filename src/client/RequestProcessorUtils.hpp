#ifndef REQUEST_PROCESSOR_UTILS_HPP
#define REQUEST_PROCESSOR_UTILS_HPP

#include <string>
#include <vector>

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"

const ServerConfig* selectServerByPort(
    int port, const std::vector<ServerConfig>* configs);

const LocationConfig* matchLocation(const ServerConfig& server,
                                    const std::string& uri);

std::string resolvePath(const ServerConfig& server,
                        const LocationConfig* location, const std::string& uri);

bool isCgiRequest(const std::string& path);
bool isCgiRequestByConfig(const LocationConfig* location,
                          const std::string& path);

std::string getFileExtension(const std::string& path);

std::string methodToString(HttpMethod method);

int validateLocation(const HttpRequest& request, const ServerConfig* server,
                     const LocationConfig* location);

#endif  // REQUEST_PROCESSOR_UTILS_HPP
