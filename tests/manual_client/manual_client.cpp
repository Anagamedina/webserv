#include <iostream>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <unistd.h>

#include "../../src/client/Client.hpp"
#include "../../src/config/ServerConfig.hpp"

int main()
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
    {
        std::cerr << "socketpair failed" << std::endl;
        return 1;
    }

    std::vector<ServerConfig> dummyConfigs;
    Client client(fds[0], &dummyConfigs, 8080);

    const std::string request =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    if (write(fds[1], request.c_str(), request.size()) == -1)
    {
        std::cerr << "write failed" << std::endl;
        close(fds[0]);
        close(fds[1]);
        return 1;
    }

    client.handleRead();
    if (client.needsWrite())
        client.handleWrite();

    char buffer[4096];
    ssize_t n = read(fds[1], buffer, sizeof(buffer));
    if (n > 0)
        std::cout.write(buffer, n);

    std::cout << std::endl;

    close(fds[0]);
    close(fds[1]);
    return 0;
}
