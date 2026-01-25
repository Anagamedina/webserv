#ifndef TCPLISTENER_HPP
#define TCPLISTENER_HPP

#include <string>

/**
 * TcpListener: Crea y gestiona el socket de escucha del servidor
 * 
 * ¿Qué hace?
 * - Crea un socket TCP
 * - Lo enlaza (bind) a una dirección IP y puerto
 * - Lo pone en modo "escucha" (listen) para aceptar conexiones
 * - Lo configura en modo NO BLOQUEANTE (crucial para epoll)
 * 
 * ¿Por qué no bloqueante?
 * - En modo bloqueante, accept() esperaría hasta que llegue una conexión
 * - Con epoll, queremos que accept() retorne inmediatamente si no hay conexiones
 * - epoll nos avisará cuando haya una conexión nueva lista
 */
class TcpListener {
public:
	TcpListener();
	~TcpListener();

	/**
	 * Enlaza el socket a una dirección y puerto, y comienza a escuchar
	 * 
	 * Proceso:
	 * 1. Crea el socket
	 * 2. Configura opciones (SO_REUSEADDR para evitar "Address already in use")
	 * 3. Lo pone en modo no bloqueante
	 * 4. Hace bind() - enlaza a la IP:puerto
	 * 5. Hace listen() - comienza a escuchar conexiones entrantes
	 */
	void bindAndListen(const std::string& host, int port);
	
	/**
	 * Acepta una nueva conexión de cliente
	 * 
	 * IMPORTANTE: En modo no bloqueante:
	 * - Si hay una conexión lista: retorna el fd del nuevo cliente
	 * - Si NO hay conexiones: retorna -1 (no bloquea esperando)
	 * 
	 * ¿Cuándo se llama?
	 * - Cuando epoll nos notifica que el socket de escucha tiene EPOLLIN
	 * - Significa que hay una conexión nueva esperando ser aceptada
	 * 
	 * @return: File descriptor del cliente, o -1 si no hay conexiones disponibles
	 */
	int acceptConnection();
	
	// Get the listener socket file descriptor
	// Necesario para agregarlo a epoll
	int getFd() const { return listenFd_; }
	
	/**
	 * Configura el socket en modo NO BLOQUEANTE
	 * 
	 * ¿Qué significa no bloqueante?
	 * - Las operaciones (accept, read, write) retornan inmediatamente
	 * - Si no pueden completarse, retornan error EAGAIN/EWOULDBLOCK
	 * - Nunca esperan/bloquean esperando que haya datos o conexiones
	 * 
	 * ¿Por qué es necesario?
	 * - epoll necesita que los sockets sean no bloqueantes
	 * - Permite que el servidor maneje múltiples clientes sin bloquearse
	 */
	void setNonBlocking();

private:
	// No copy constructor or assignment operator (C++98 style)
	TcpListener(const TcpListener&);
	TcpListener& operator=(const TcpListener&);

	int listenFd_;      // File descriptor del socket de escucha
	bool isBound_;      // Flag para saber si ya está enlazado
	
	void createSocket();        // Crea el socket TCP
	void setSocketOptions();    // Configura opciones del socket (SO_REUSEADDR)
};

#endif // TCPLISTENER_HPP

