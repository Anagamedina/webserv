#include <signal.h>

#include <exception>
#include <iostream>
#include <string>

#include "common/namespaces.hpp"
#include "config/ConfigException.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigUtils.hpp"
#include "network/ServerManager.hpp"

/**
 * Main function for the web server.
 *
 * Execution flow:
 * 1. Configure signals (SIGPIPE).
 * 2. Create the ServerManager.
 * 3. Start the server on the configured ports.
 * 4. Run the event loop (blocks here until the process terminates).
 */
int main(int argc, char* argv[]) {
  const std::string configPath =
      (argc > 1) ? argv[1] : config::paths::default_config_path;

  if (!config::utils::fileExists(configPath)) {
    std::cerr << "Error: Config file: '" << configPath
              << "'\nPlease ensure:\n\t1. The file exists\n\t2. You have read "
                 "permissions\n\t3. You are running from the project root: "
                 "./webserver\n";
    return 1;
  }

  /**
   * Disable SIGPIPE to avoid crashes.
   *
   * What is SIGPIPE?
   * - It is sent when writing to a socket that has been closed by the other
   * end.
   * - By default, it terminates the process.
   * - With SIG_IGN, we ignore it, and write() simply returns an error.
   *
   * Why is it necessary?
   * - In a server, clients can disconnect at any time.
   * - We don't want the server to crash when trying to write to a closed
   * socket.
   * - Especially important for CGI (when the child process terminates).
   */
  signal(SIGPIPE, SIG_IGN);

  try {
    ConfigParser parser(configPath);
    std::cout << "Config file path: [" << config::colors::blue
              << parser.getConfigFilePath() << "]\n"
              << config::colors::reset;
    parser.parse();

    // Create the server manager with the list of server configurations.
    ServerManager server(&parser.getServers());

    /**
     * Start the server(s).
     *
     * This handles:
     * 1. Creating listening sockets.
     * 2. Binding to the specified host:port.
     * 3. Placing them in listen mode.
     * 4. Registering them with epoll to monitor for new connections.
     */

    /**
     * Run the main event loop.
     *
     * IMPORTANT: This function BLOCKS and DOES NOT RETURN under normal
     * circumstances.
     *
     * The loop:
     * 1. Waits for events with epoll_wait() (blocks here).
     * 2. When events occur, it processes them.
     * 3. Loops back to wait for more events.
     *
     * The server runs until:
     * - Ctrl+C (SIGINT) is pressed.
     * - The process is killed.
     * - A fatal error occurs.
     */
    server.run();
  } catch (const ConfigException& e) {
    // Specific handling for configuration-related errors.
    std::cerr << "Configuration error: " << e.what() << std::endl;
    return 1;
  } catch (const std::exception& e) {
    // Catch-all for any other standard exceptions during initialization or
    // execution.
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  std::flush(std::cout);
  return 0;
}
