#pragma once

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../config/ServerConfig.hpp"
#include <string>
#include <map>

class CgiExecutor {
public:
    CgiExecutor();
    ~CgiExecutor();

    // Executes the CGI script and returns the raw output (headers + body)
    // Or returns an HttpResponse directly?
    // Better to return std::string output, and let caller parse it?
    // Or parse it here? 
    // Let's return HttpResponse to encapsulate the logic.
    HttpResponse execute(const HttpRequest& request, const std::string& script_path, const std::string& interpreter_path);

private:
    std::map<std::string, std::string> prepareEnvironment(const HttpRequest& request, const std::string& script_path);
    char** createEnvArray(const std::map<std::string, std::string>& env_map);
    void freeEnvArray(char** env_array);
};
