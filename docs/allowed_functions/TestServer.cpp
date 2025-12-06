#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>

struct sockaddr {
	sa_family_t sa_family;   // AF_INET, AF_UNIX, etc.
	char        sa_data[14]; // datos específicos del dominio
};

int main()
{
	//	creating a socket
	/*
	 * SOCK_STREAM (TCP, fiable)
	 * SOCK_DGRAM (UDP, datagramas)
	 * protocol is 0 because the kernel choose the more efficient
	*/

	int sockFd = socket(AF_UNIX | AF_INET, SOCK_STREAM, 0);

	if (sockFd == -1)
	{
		std::cerr << "Error creating socket. :(" << std::endl;
	}

	// bind(), asignamos una direccion IP o puerto al socket
	// struct sockaddr_in srv;
	struct sockaddr_in srv = {};
	srv.sin_family = AF_INET;
	srv.sin_port   = htons(8080);
	inet_pton(AF_INET, "0.0.0.0", &srv.sin_addr);

	// bind(sockFd, (struct sockaddr*)&srv, sizeof(srv));
	bind(sockFd, &srv, sizeof(srv));

	return 0;
}
