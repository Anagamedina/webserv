#pragma once

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../config/ServerConfig.hpp"
#include "../cgi/CgiExecutor.hpp"
#include <string>

class RequestProcessor {
public:
    RequestProcessor();
    ~RequestProcessor();

    HttpResponse process(const HttpRequest& request, const ServerBlock* config);

private:
    // Helpers
    const LocationConfig* findLocation(const std::string& uri, const ServerBlock* config);
    std::string resolvePath(const std::string& uri, const LocationConfig& location, const ServerBlock* config);
    
    // Handlers
    HttpResponse handleStaticFile(const std::string& path);
    HttpResponse handleDirectory(const std::string& path, const LocationConfig& location);
    HttpResponse handleError(int code, const ServerBlock* config);
    HttpResponse handleRedirect(const std::string& location_url);
    
    // New Handlers
    HttpResponse handleDelete(const std::string& path, const ServerBlock* config);
    HttpResponse handlePost(const HttpRequest& request, const std::string& path, const LocationConfig& location, const ServerBlock* config);

    // Utils
    std::string getMimeType(const std::string& path);
    bool methodAllowed(const std::string& method, const LocationConfig& location);
    
    CgiExecutor cgi_executor_;
};
