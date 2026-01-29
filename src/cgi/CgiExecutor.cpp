#include "CgiExecutor.hpp"
#include "../utils/StringUtils.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

CgiExecutor::CgiExecutor() {}

CgiExecutor::~CgiExecutor() {}

HttpResponse CgiExecutor::execute(const HttpRequest &request,
                                  const std::string &script_path,
                                  const std::string &interpreter_path) {
  int pipe_in[2];  // Parent writes to Child (Request Body)
  int pipe_out[2]; // Child writes to Parent (Response)

  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    return HttpResponse(500);
  }

  pid_t pid = fork();
  if (pid == -1) {
    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);
    return HttpResponse(500);
  }

  if (pid == 0) {
    // CHILD PROCESS

    // Setup pipes
    dup2(pipe_in[0], STDIN_FILENO);
    dup2(pipe_out[1], STDOUT_FILENO);

    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);

    // Timeout safety
    alarm(5);

    // Prepare Env
    std::map<std::string, std::string> env_map =
        prepareEnvironment(request, script_path);
    char **envp = createEnvArray(env_map);

    // Prepare Args
    // If interpreter is set (e.g. /usr/bin/php-cgi), args are [interpreter,
    // script_path, NULL] If no interpreter (executable script), args are
    // [script_path, NULL]

    char *args[3];
    if (!interpreter_path.empty()) {
      args[0] = strdup(interpreter_path.c_str());
      args[1] = strdup(script_path.c_str());
      args[2] = NULL;
    } else {
      args[0] = strdup(script_path.c_str());
      args[1] = NULL;
      args[2] = NULL;
    }

    const char *exec_path = args[0];
    execve(exec_path, args, envp);

    // If execve fails
    perror("execve failed");
    exit(1);

  } else {
    // PARENT PROCESS

    close(pipe_in[0]);  // Close read end of input pipe
    close(pipe_out[1]); // Close write end of output pipe

    // Write Request Body to Child
    std::string body = request.getBody();
    if (!body.empty()) {
      write(pipe_in[1], body.c_str(), body.length());
    }
    close(pipe_in[1]); // Close write end (EOF for child)

    // Read Response from Child
    std::string obj_response;
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0) {
      obj_response.append(buffer, bytes);
    }
    close(pipe_out[0]);

    // Wait for child
    int status;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status)) {
      if (WTERMSIG(status) == SIGALRM) {
        return HttpResponse(504); // Gateway Timeout
      }
      return HttpResponse(500);
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      return HttpResponse(502); // Bad Gateway (CGI failed)
    }

    // 3. Parse CGI Output
    // CGI output normally contains headers separated by \n (or \r\n) and then
    // body. We need to construct an HttpResponse.

    HttpResponse response(200); // Default, might be overridden by Status header

    size_t header_end = obj_response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
      header_end = obj_response.find("\n\n"); // Loose check
    }

    if (header_end != std::string::npos) {
      std::string headers = obj_response.substr(0, header_end);
      std::string body_content = obj_response.substr(
          header_end + ((obj_response[header_end] == '\r') ? 4 : 2));

      response.setBody(body_content);

      // Parse headers
      std::istringstream iss(headers);
      std::string line;
      while (std::getline(iss, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r')
          line.erase(line.length() - 1);
        if (line.empty())
          continue;

        size_t colon = line.find(':');
        if (colon != std::string::npos) {
          std::string key = line.substr(0, colon);
          std::string value = line.substr(colon + 1);
          // Trim value
          size_t first = value.find_first_not_of(" \t");
          if (first != std::string::npos)
            value = value.substr(first);

          std::string key_lower = key;
          std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(),
                         ::tolower);

          if (key_lower == "status") {
            // Parse status code (e.g. "Status: 404 Not Found")
            int code = std::atoi(value.c_str());
            response.setStatus(code);
          } else {
            response.setHeader(key, value);
          }
        }
      }
    } else {
      // No headers found? treat generic
      response.setBody(obj_response);
    }

    return response;
  }
}

std::map<std::string, std::string>
CgiExecutor::prepareEnvironment(const HttpRequest &request,
                                const std::string &script_path) {
  std::map<std::string, std::string> env;

  env["GATEWAY_INTERFACE"] = "CGI/1.1";
  env["SERVER_PROTOCOL"] = "HTTP/1.1";
  env["SERVER_SOFTWARE"] = "Webserv/1.0";
  env["REQUEST_METHOD"] = request.getMethod();
  env["SCRIPT_FILENAME"] = script_path;
  env["SCRIPT_NAME"] = script_path; // Relative path?
  env["QUERY_STRING"] = "";         // TODO: parse query string from URI
  env["REMOTE_ADDR"] = "127.0.0.1"; // TODO: pass client IP

  // Headers
  env["CONTENT_LENGTH"] = StringUtils::toString(request.getBody().length());
  std::string ct = request.getHeader("content-type");
  if (!ct.empty())
    env["CONTENT_TYPE"] = ct;

  // Pass other headers as HTTP_...
  const std::map<std::string, std::string> &headers = request.getHeaders();
  for (std::map<std::string, std::string>::const_iterator it = headers.begin();
       it != headers.end(); ++it) {
    std::string key = it->first;
    // Upper case and replace - with _
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

char **
CgiExecutor::createEnvArray(const std::map<std::string, std::string> &env_map) {
  char **envp = new char *[env_map.size() + 1];
  int i = 0;
  for (std::map<std::string, std::string>::const_iterator it = env_map.begin();
       it != env_map.end(); ++it) {
    std::string s = it->first + "=" + it->second;
    envp[i++] = strdup(s.c_str());
  }
  envp[i] = NULL;
  return envp;
}
