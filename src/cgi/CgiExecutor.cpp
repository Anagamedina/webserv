/**
 * CgiExecutor.cpp
 *
 * Asynchronous CGI execution implementation
 *
 * Key design decisions:
 * 1. Non-blocking: Fork immediately, don't wait for output
 * 2. Pipes are non-blocking for reading/writing
 * 3. Monitored via epoll in main server loop
 * 4. Supports streaming responses as data becomes available
 * 5. Timeout enforcement via SIGALRM in child
 */

#include "CgiExecutor.hpp"

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

static std::string methodToString(HttpMethod method) {
  if (method == HTTP_METHOD_GET) return "GET";
  if (method == HTTP_METHOD_POST) return "POST";
  if (method == HTTP_METHOD_DELETE) return "DELETE";
  return "";
}

static std::string bodyToString(const std::vector<char>& body) {
  return std::string(body.begin(), body.end());
}

CgiExecutor::CgiExecutor() {}

CgiExecutor::~CgiExecutor() {}

CgiProcess* CgiExecutor::executeAsync(const HttpRequest& request,
                                      const std::string& script_path,
                                      const std::string& interpreter_path,
                                      const ServerConfig& serverConfig) {
  // Step 1: Create communication pipes
  // pipe_in: parent writes request body to child stdin
  // pipe_out: parent reads CGI output from child stdout

  int pipe_in[2];   // Parent → Child (request body)
  int pipe_out[2];  // Child → Parent (response)

  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    std::cerr << "Failed to create pipes for CGI" << std::endl;
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);
    return NULL;
  }

  // Step 2: Make pipes non-blocking
  // This prevents the main event loop from blocking on pipe I/O

  if (!setNonBlocking(pipe_in[1]) || !setNonBlocking(pipe_out[0])) {
    std::cerr << "Failed to set pipes non-blocking" << std::endl;
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);
    return NULL;
  }

  // Step 3: Fork child process

  pid_t pid = fork();
  if (pid == -1) {
    std::cerr << "Failed to fork CGI process" << std::endl;
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);
    return NULL;
  }

  if (pid == 0) {
    // CHILD PROCESS

    // Setup pipes for stdin/stdout
    dup2(pipe_in[0], STDIN_FILENO);
    dup2(pipe_out[1], STDOUT_FILENO);

    // Close all pipe ends in child
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);

    // Extract script directory and filename
    std::string script_dir = ".";
    std::string script_name = script_path;
    size_t last_slash = script_path.find_last_of('/');
    if (last_slash != std::string::npos) {
      script_dir = script_path.substr(0, last_slash);
      script_name = script_path.substr(last_slash + 1);
    }

    // Change to script directory
    if (chdir(script_dir.c_str()) == -1) {
      std::cerr << "chdir failed: " << std::strerror(errno) << std::endl;
      kill(getpid(), SIGKILL);
    }

    // Prepare environment variables
    // INFO: Use full path for SCRIPT_FILENAME env var
    std::map<std::string, std::string> env_map =
        prepareEnvironment(request, script_path, serverConfig);
#ifdef DEBUG
    std::cout << "[CGI ENV] script=" << script_path << std::endl;
    for (std::map<std::string, std::string>::const_iterator it =
             env_map.begin();
         it != env_map.end(); ++it) {
      std::cerr << "[CGI ENV] " << it->first << "=" << it->second << std::endl;
    }
#endif
    char** envp = createEnvArray(env_map);

    // Prepare arguments - use just the script filename after chdir
    // Prefix with ./ for relative paths to work with /usr/bin/env and direct
    // execution
    std::string relative_script = "./" + script_name;
    // We use a vector to hold the allocated pointers so we can track them if
    // needed, though in a successful execve they are replaced, and on failure
    // we kill the process. so we do not need to delete them manually since the
    // process will wipe memory it self

    char* args[3];
    args[0] = NULL;
    args[1] = NULL;
    args[2] = NULL;

    if (!interpreter_path.empty()) {
      args[0] = new char[interpreter_path.size() + 1];
      std::strcpy(args[0], interpreter_path.c_str());

      args[1] = new char[relative_script.size() + 1];
      std::strcpy(args[1], relative_script.c_str());
    } else {
      args[0] = new char[relative_script.size() + 1];
      std::strcpy(args[0], relative_script.c_str());
    }

    const char* exec_path = args[0];
    execve(exec_path, args, envp);

    // If execve fails
    std::cerr << "execve failed: " << std::strerror(errno) << std::endl;
    kill(getpid(), SIGKILL);

  } else {
    // PARENT PROCESS

    // Close unused pipe ends
    close(pipe_in[0]);   // Don't read from input pipe
    close(pipe_out[1]);  // Don't write to output pipe

    // Write request body to child stdin
    // Handled asynchronously by Client/ServerManager via CgiProcess

    // Create CgiProcess tracker object
    // The Client will own this and clean it up when done
    std::string body = bodyToString(request.getBody());
    CgiProcess* proc =
        new CgiProcess(script_path, interpreter_path,
                       pipe_in[1],           // Pass write end to CgiProcess
                       pipe_out[0], pid, 5,  // 5 second timeout
                       body);

    return proc;
  }

  return NULL;  // Should not reach here
}

std::map<std::string, std::string> CgiExecutor::prepareEnvironment(
    const HttpRequest& request, const std::string& script_path,
    const ServerConfig& serverConfig) {
  std::map<std::string, std::string> env;

  // Step 1: Core CGI/HTTP Variables

  env["GATEWAY_INTERFACE"] = "CGI/1.1";
  env["SERVER_PROTOCOL"] = "HTTP/1.1";
  env["SERVER_SOFTWARE"] = "Webserv/1.0";
  env["REQUEST_METHOD"] = methodToString(request.getMethod());

  // Step 2: Path and Script Variables

  env["SCRIPT_FILENAME"] = script_path;

  std::string uri = request.getPath();
  if (!request.getQuery().empty()) {
    uri += "?";
    uri += request.getQuery();
  }
  size_t question_mark = uri.find('?');
  std::string script_name =
      (question_mark != std::string::npos) ? uri.substr(0, question_mark) : uri;
  env["SCRIPT_NAME"] = script_name;

  // Step 3: Query String
  // QUERY_STRING: Everything after the '?' in the URI

  std::string query_string = "";
  if (question_mark != std::string::npos && question_mark + 1 < uri.length()) {
    query_string = uri.substr(question_mark + 1);
  }
  env["QUERY_STRING"] = query_string;

  // Step 4: Content/Body Information

  std::ostringstream len;
  len << request.getBody().size();
  env["CONTENT_LENGTH"] = len.str();
  std::string ct = request.getHeader("content-type");
  if (!ct.empty()) env["CONTENT_TYPE"] = ct;

  // Step 5: Client/Server Connection Information

  env["REMOTE_ADDR"] =
      "127.0.0.1";  // TODO: Extract from socket via getpeername()
  env["REQUEST_URI"] = uri;

  // Step 5b: Server identification (CGI/1.1 spec)
  env["SERVER_NAME"] = serverConfig.getServerName();
  std::ostringstream port_ss;
  port_ss << serverConfig.getPort();
  env["SERVER_PORT"] = port_ss.str();
  env["PATH_INFO"] =
      request.getPath();  // We only support direct script execution for now
  env["PATH_TRANSLATED"] = "";     // Corresponding physical path
  env["REDIRECT_STATUS"] = "200";  // Required by php-cgi in some setups

  // Step 6: HTTP Request Headers as HTTP_* variables

  const std::map<std::string, std::string>& headers = request.getHeaders();
  for (std::map<std::string, std::string>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    std::string key = it->first;
    std::string env_key = "HTTP_";
    for (size_t i = 0; i < key.length(); ++i) {
      char c = key[i];
      if (c == '-')
        env_key += '_';
      else
        env_key += toupper(c);
    }
    env[env_key] = it->second;
  }

  return env;
}

char** CgiExecutor::createEnvArray(
    const std::map<std::string, std::string>& env_map) {
  char** envp = new char*[env_map.size() + 1];
  int i = 0;
  for (std::map<std::string, std::string>::const_iterator it = env_map.begin();
       it != env_map.end(); ++it) {
    std::string s = it->first + "=" + it->second;
    envp[i] = new char[s.size() + 1];
    std::strcpy(envp[i], s.c_str());
    i++;
  }
  envp[i] = NULL;
  return envp;
}

bool CgiExecutor::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return false;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}
