#include <signal.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "config/ConfigException.hpp"
#include "config/ConfigParser.hpp"
#include "config/ServerConfig.hpp"
#include "network/ServerManager.hpp"

/**
 * Función principal del servidor web
 *
 * Flujo de ejecución:
 * 1. Configura señales (SIGPIPE)
 * 2. Crea el ServerManager
 * 3. Inicia el servidor en un puerto
 * 4. Ejecuta el bucle de eventos (bloquea aquí hasta que termine el proceso)
 */
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
  /**
   * Deshabilitar SIGPIPE para evitar crashes
   *
   * ¿Qué es SIGPIPE?
   * - Se envía cuando escribes a un socket que el otro extremo cerró
   * - Por defecto, mata el proceso
   * - Con SIG_IGN, lo ignoramos y write() simplemente retorna error
   *
   * ¿Por qué es necesario?
   * - En un servidor, los clientes pueden desconectarse en cualquier momento
   * - Si intentamos escribir a un socket cerrado, no queremos que el servidor
   * crashee
   * - Especialmente importante para CGI (cuando el proceso hijo se cierra)
   */
  signal(SIGPIPE, SIG_IGN);

  std::cout << "Esto se pone interesante" << std::endl;

  try {
    ConfigParser parser(configPath);
    std::cout << "Config file path: [" << parser.getConfigFilePath() << "]\n";
    parser.parse();
    // - ConfigParser parser("config/default.conf");
    // - parser.parse();
    // - std::vector<ServerConfig> servers = parser.getServers();
    // - ServerManager server(&servers);

    // Crear el gestor del servidor con la lista de servers
    ServerManager server(&parser.getServers());

    /**
     * Iniciar el servidor en localhost:8080
     *
     * Esto:
     * 1. Crea el socket de escucha
     * 2. Lo enlaza a 127.0.0.1:8080
     * 3. Lo pone en modo escucha
     * 4. Lo registra en epoll para monitorear conexiones nuevas
     *
     * El puerto 8080 coincide con el default.conf
     */
    /**
     * Ejecutar el bucle principal de eventos
     *
     * IMPORTANTE: Esta función BLOQUEA y NO RETORNA
     *
     * El bucle:
     * 1. Espera eventos con epoll_wait() (bloquea aquí)
     * 2. Cuando hay eventos, los procesa
     * 3. Vuelve a esperar más eventos
     *
     * El servidor corre hasta que:
     * - Se presiona Ctrl+C (SIGINT)
     * - Se mata el proceso (kill)
     * - Ocurre un error fatal
     */
    server.run();
  } /*catch (std::exception& e) {
          // Si algo sale mal durante la inicialización, mostrar el error
          std::cerr << "Error: " << e.what() << std::endl;
          return 1;
  }*/
  /*catch (const ConfigException& e)
  {
          std::cerr << "Configuration msg_errors: " << e.what() << std::endl;
          return 1;
  }*/
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  std::flush(std::cout);
  return 0;
}
