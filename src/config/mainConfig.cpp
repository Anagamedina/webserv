#include <fstream>
#include <iostream>
#include <string>

#include "../common/namespaces.hpp"
#include "ConfigException.hpp"
#include "ConfigParser.hpp"
#include "ConfigUtils.hpp"

int main(int argc, char* argv[]) {
  const std::string configPath =
      (argc > 1) ? argv[1] : config::paths::default_config_path;

  if (!config::utils::fileExists(configPath)) {
    std::cerr
        << "Error: Config file: '" << configPath
        << "'\nPlease ensure:\n\t1. The file exists\n\t2. You have read "
           "permissions\n\t3. You are running from project root: ./webserver\n";
    return 1;
  }
  try {
    ConfigParser parser(configPath);
    std::cout << "Config file path: [" << parser.getConfigFilePath() << "]\n";
    parser.parse();

    /**
    // // Get parsed servers
    // const std::vector<ServerConfig>& servers = parser.getServers();
    // std::cout << "✓ Successfully loaded " << servers.size() << " server(s)"
    // 	<< std::endl;

    // Debug: print server
    for (size_t i = 0; i < servers.size(); ++i)
    {
            std::cout << "\n--- Server " << (i + 1) << " ---" << std::endl;
            std::cout << servers[i];
    }
*/
  } catch (const ConfigException& e) {
    std::cerr << "Configuration msg_errors: " << e.what() << std::endl;
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  std::flush(std::cout);
  return 0;
}
