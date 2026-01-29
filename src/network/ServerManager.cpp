#include "ServerManager.hpp"
#include <unistd.h>
#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <cstring>

ServerManager::ServerManager() {
}

ServerManager::~ServerManager() {
	// Cerrar todas las conexiones de clientes al destruir el servidor
	for (std::map<int, Client*>::iterator it = clients_.begin(); it != clients_.end(); ++it) {
		close(it->first);
		delete it->second;
	}
	clients_.clear();
}

/**
 * Inicia el servidor: crea el socket de escucha y lo registra en epoll
 * 
 * Proceso:
 * 1. listener_.bindAndListen(): Crea y configura el socket de escucha
 * 2. epoll_.addFd(): Registra el socket en epoll para monitorear conexiones nuevas
 * 
 * EPOLLIN: Queremos ser notificados cuando haya datos para leer
 *          En el socket de escucha, esto significa "hay una conexión nueva"
 * 
 * Level-triggered (LT):
 *          - Notifica mientras el socket siga listo
 *          - Mas simple de manejar para lectura/escritura incremental
 */
void ServerManager::start(const std::string& host, int port) {
	// Bind and listen
	listener_.bindAndListen(host, port);
	
	// Agregar el socket de escucha a epoll (level-triggered)
	// EPOLLIN: Notificar cuando haya conexiones nuevas
	epoll_.addFd(listener_.getFd(), EPOLLIN);
	
	std::cout << "Server started and ready to accept connections" << std::endl;
}

/**
 * BUCLE PRINCIPAL DEL SERVIDOR - El corazón de todo
 * 
 * Este es el bucle de eventos que hace que el servidor funcione.
 * Es la implementación del patrón "event loop" o "reactor pattern".
 * 
 * ¿Cómo funciona?
 * 
 * 1. ESPERAR EVENTOS (epoll_wait):
 *    - Bloquea aquí hasta que ocurra algo (conexión nueva, datos recibidos, etc.)
 *    - El kernel despierta nuestro proceso cuando hay eventos
 *    - Retorna una lista de TODOS los eventos que ocurrieron
 * 
 * 2. PROCESAR EVENTOS:
 *    - Iteramos sobre cada evento recibido
 *    - Identificamos qué socket es (listener o cliente)
 *    - Llamamos a la función apropiada para manejar ese evento
 * 
 * 3. REPETIR:
 *    - Volvemos a esperar más eventos
 *    - El ciclo continúa indefinidamente
 * 
 * ¿Por qué es eficiente?
 * - Solo bloquea cuando NO hay nada que hacer (ahorra CPU)
 * - Cuando hay eventos, los procesa rápidamente (todo no bloqueante)
 * - Puede manejar miles de clientes con un solo hilo
 * 
 * ¿Qué tipos de eventos procesamos?
 * - EPOLLIN en listener: Nueva conexión entrante
 * - EPOLLIN en cliente: Datos recibidos del cliente
 * - EPOLLERR/EPOLLHUP: Error o cliente desconectado
 */
void ServerManager::run() {
	std::vector<struct epoll_event> events;
	
	std::cout << "Entering event loop..." << std::endl;
	
	// BUCLE INFINITO - El servidor corre hasta que se termine el proceso
	while (true) {
		// PASO 1: ESPERAR EVENTOS
		// Esta es la ÚNICA línea que bloquea en todo el servidor
		// timeout = -1 significa "esperar indefinidamente hasta que haya eventos"
		int numEvents = epoll_.wait(events, -1);
		
		if (numEvents == 0) {
			continue; // Timeout (no debería pasar con timeout=-1, pero por si acaso)
		}
		
		// PASO 2: PROCESAR CADA EVENTO
		// Puede haber múltiples eventos listos al mismo tiempo
		for (int i = 0; i < numEvents; ++i) {
			int fd = events[i].data.fd;  // El fd que generó este evento
			
			// ¿Es el socket de escucha? → Nueva conexión
			if (fd == listener_.getFd()) {
				handleNewConnection();
			}
			// ¿Hubo error o el cliente cerró? → Limpiar conexión
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				handleClientDisconnect(fd);
				continue;
			}
			// ¿Es un cliente con datos para leer? → Leer datos
			if (events[i].events & EPOLLIN) {
				handleClientData(fd);
			}
			// ¿Es un cliente con datos para escribir? → Enviar respuesta
			if (events[i].events & EPOLLOUT) {
				std::map<int, Client*>::iterator it = clients_.find(fd);
				if (it != clients_.end())
					it->second->handleWrite();
			}

			// Si el cliente cerró, limpiar
			std::map<int, Client*>::iterator it = clients_.find(fd);
			if (it != clients_.end() && it->second->getState() == STATE_CLOSED) {
				handleClientDisconnect(fd);
			}
		}
		
		// PASO 3: Volver a esperar más eventos (vuelta al inicio del while)
	}
}

/**
 * Maneja nuevas conexiones entrantes
 * 
 * Se llama cuando epoll detecta EPOLLIN en el socket de escucha.
 * Esto significa que hay al menos una conexión nueva esperando ser aceptada.
 * 
 * En level-triggered basta con aceptar una conexion por evento.
 */
void ServerManager::handleNewConnection() {
	int clientFd = listener_.acceptConnection();
	if (clientFd == -1)
		return;

	// Registrar el nuevo cliente en epoll para monitorear sus datos
	// EPOLLIN: Notificar cuando el cliente envíe datos
	// EPOLLRDHUP: Notificar cuando el cliente cierre la conexión
	epoll_.addFd(clientFd, EPOLLIN | EPOLLRDHUP);

	// Guardar el fd del cliente para poder rastrearlo
	clients_[clientFd] = new Client(clientFd, 0);
}

/**
 * Maneja datos recibidos de un cliente
 * 
 * Se llama cuando epoll detecta EPOLLIN en un socket de cliente.
 * Esto significa que el cliente envió datos y están listos para leer.
 * 
 * ¿Qué hace recv()?
 * - Lee datos del socket del cliente
 * - En modo no bloqueante:
 *   - bytesRead > 0: Leyó datos (puede ser menos que el tamaño del buffer)
 *   - bytesRead == 0: Cliente cerró la conexión
 *   - bytesRead == -1: Error de lectura
 */
void ServerManager::handleClientData(int clientFd) {
	std::map<int, Client*>::iterator it = clients_.find(clientFd);
	if (it == clients_.end())
		return;

	it->second->handleRead();

	if (it->second->getState() == STATE_CLOSED) {
		handleClientDisconnect(clientFd);
		return;
	}

	uint32_t events = EPOLLIN | EPOLLRDHUP;
	if (it->second->needsWrite())
		events |= EPOLLOUT;
	epoll_.modifyFd(clientFd, events);
}

/**
 * Maneja la desconexión de un cliente
 * 
 * Se llama cuando:
 * - El cliente cierra la conexión (read() retorna 0)
 * - Hay un error en el socket (EPOLLERR)
 * - El cliente cierra su extremo (EPOLLRDHUP)
 * 
 * Proceso de limpieza:
 * 1. Remover el socket de epoll (ya no queremos monitorearlo)
 * 2. Cerrar el file descriptor (liberar recursos del kernel)
 * 3. Remover del mapa de clientes (limpiar nuestra estructura de datos)
 * 
 * ¿Por qué es importante limpiar?
 * - Los file descriptors son recursos limitados del sistema
 * - Si no los cerramos, se agotan (límite típico: 1024 por proceso)
 * - Si no los removemos de epoll, epoll seguirá monitoreando sockets muertos
 */
void ServerManager::handleClientDisconnect(int clientFd) {
	std::cout << "Client " << clientFd << " disconnected" << std::endl;
	
	// Remover de epoll (ya no queremos monitorear este socket)
	epoll_.removeFd(clientFd);
	
	// Cerrar el socket (liberar el file descriptor)
	close(clientFd);
	
	// Remover del mapa de clientes (limpiar nuestra estructura de datos)
	std::map<int, Client*>::iterator it = clients_.find(clientFd);
	if (it != clients_.end()) {
		delete it->second;
		clients_.erase(it);
	}
}

