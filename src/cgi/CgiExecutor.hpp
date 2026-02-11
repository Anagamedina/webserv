/**
 * CgiExecutor.hpp
 *
 * Asynchronous CGI execution
 * Forks CGI process without blocking, returns immediately
 * Pipes are monitored via epoll by the main server loop
 */

#pragma once

#include <map>
#include <string>

#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "CgiProcess.hpp"

class CgiExecutor {
 public:
  CgiExecutor();
  ~CgiExecutor();

  /**
   * Start asynchronous CGI execution
   *
   * Forks child process, sets up pipes, returns immediately
   * The CGI process output is monitored via epoll
   *
   * @param request: HTTP request from client
   * @param script_path: Full path to CGI script
   * @param interpreter_path: Path to interpreter (empty for executable scripts)
   * @return Pointer to CgiProcess to track execution
   *         NULL if fork/pipe creation failed
   */
  CgiProcess* executeAsync(const HttpRequest& request,
                           const std::string& script_path,
                           const std::string& interpreter_path);

 private:
  /**
   * Prepare environment variables for CGI
   *
   * @param request: HTTP request
   * @param script_path: CGI script path
   * @return Map of environment variables
   */
  std::map<std::string, std::string> prepareEnvironment(
      const HttpRequest& request, const std::string& script_path);

  /**
   * Convert environment map to C-style array for execve
   *
   * @param env_map: Environment variable map
   * @return Allocated char** array (caller must free)
   */
  char** createEnvArray(const std::map<std::string, std::string>& env_map);

  /**
   * Set up a pipe as non-blocking
   *
   * @param fd: File descriptor
   * @return true on success, false on error
   */
  bool setNonBlocking(int fd);
};
