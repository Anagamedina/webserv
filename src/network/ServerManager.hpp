#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "TcpListener.hpp"
#include "EpollWrapper.hpp"
#include "../client/Client.hpp"
#include "../config/ServerConfig.hpp"
#include <vector>
#include <map>
#include <string>

/**
 * ServerManager: Coordina TcpListener y EpollWrapper para crear el servidor
 * 
 * Esta es la clase que orquesta todo:
 * - Crea el socket de escucha (TcpListener)
 * - Crea el mecanismo de eventos (EpollWrapper)
 * - Ejecuta el bucle principal de eventos
 * - Maneja conexiones nuevas, datos recibidos, y desconexiones
 * 
 * Flujo del servidor:
 * 1. start(): Configura el servidor y lo pone a escuchar
 * 2. run(): Bucle infinito que:
 *    a) Espera eventos con epoll_wait() (BLOQUEA aquí hasta que haya eventos)
 *    b) Procesa cada evento (nueva conexión, datos recibidos, error)
 *    c) Vuelve a esperar más eventos
 * 
 * ¿Por qué es eficiente?
 * - Solo bloquea en epoll_wait() cuando NO hay nada que hacer
 * - Cuando hay eventos, los procesa rápidamente (todo es no bloqueante)
 * - Puede manejar miles de clientes simultáneos con un solo hilo
 */
class ServerManager {
private:
	// No copy constructor or assignment operator (C++98 style)
	ServerManager(const ServerManager&);
	ServerManager& operator=(const ServerManager&);

	TcpListener listener_;  // Socket de escucha para nuevas conexiones
	EpollWrapper epoll_;    // Mecanismo de eventos para I/O no bloqueante
	
	// Mapa fd -> Client*
	std::map<int, Client*> clients_;

	// Configuracion compartida (ServerConfig)
	const std::vector<ServerConfig>* configs_;
	int listen_port_;
	
	/**
	 * Maneja nuevas conexiones entrantes
	 * Se llama cuando epoll detecta que el socket de escucha tiene EPOLLIN
	 */
	void handleNewConnection();
	
	/**
	 * Maneja datos recibidos de un cliente
	 * Se llama cuando epoll detecta que un socket de cliente tiene EPOLLIN
	 * 
	 * @param clientFd: File descriptor del cliente que envió datos
	 */
	void handleClientData(int clientFd);
	
	/**
	 * Maneja la desconexión de un cliente
	 * Se llama cuando:
	 * - El cliente cierra la conexión (EPOLLRDHUP)
	 * - Hay un error en el socket (EPOLLERR)
	 * - read() retorna 0 (cliente cerró)
	 * 
	 * @param clientFd: File descriptor del cliente que se desconectó
	 */
	void handleClientDisconnect(int clientFd);

public:
	ServerManager();
	~ServerManager();

	/**
	 * Inicia el servidor en la dirección y puerto especificados
	 * 
	 * Proceso:
	 * 1. Crea el socket de escucha y lo enlaza al puerto
	 * 2. Agrega el socket de escucha a epoll para monitorear conexiones nuevas
	 * 3. El servidor está listo para recibir conexiones
	 */
	void start(const std::string& host, int port);
	void setConfigs(const std::vector<ServerConfig>* configs);
	
	/**
	 * Ejecuta el bucle principal de eventos (NO RETORNA)
	 * 
	 * Este es el corazón del servidor. El bucle:
	 * 1. Espera eventos con epoll_wait() (único punto de bloqueo)
	 * 2. Cuando hay eventos, los procesa uno por uno
	 * 3. Vuelve a esperar más eventos
	 * 
	 * Se ejecuta indefinidamente hasta que el proceso sea terminado
	 */
	void run();
};

#endif // SERVERMANAGER_HPP

