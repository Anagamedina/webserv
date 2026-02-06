#include <exception>
#include <iostream>
#include <signal.h>
#include <vector>
#include "network/ServerManager.hpp"
#include "config/ServerConfig.hpp"

/**
 * Función principal del servidor web
 * 
 * Flujo de ejecución:
 * 1. Configura señales (SIGPIPE)
 * 2. Crea el ServerManager
 * 3. Inicia el servidor en un puerto
 * 4. Ejecuta el bucle de eventos (bloquea aquí hasta que termine el proceso)
 */
int main(int argc, char *argv[]) {
	((void)argc, (void)argv);
	
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
	 * - Si intentamos escribir a un socket cerrado, no queremos que el servidor crashee
	 * - Especialmente importante para CGI (cuando el proceso hijo se cierra)
	 */
	signal(SIGPIPE, SIG_IGN);

	std::cout << "Esto se pone interesante" << std::endl;

	try {
		// TODO: cuando ConfigParser este listo:
		// - ConfigParser parser("config/default.conf");
		// - parser.parse();
		// - std::vector<ServerConfig> servers = parser.getServers();
		// - ServerManager server(&servers);

		// Config mínima temporal (un solo server en 8080)
		std::vector<ServerConfig> servers;
		ServerConfig defaultServer;
		defaultServer.setPort(8080);
		servers.push_back(defaultServer);

		// Crear el gestor del servidor con la lista de servers
		ServerManager server(&servers);
		
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
		
	} catch (std::exception& e) {
		// Si algo sale mal durante la inicialización, mostrar el error
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
