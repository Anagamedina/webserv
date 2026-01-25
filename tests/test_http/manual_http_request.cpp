#include <iostream>
#include <string>
#include "../../src/http/HttpRequest.hpp"

static int g_failures = 0;

void assertTrue(bool cond, const std::string &msg)
{
    if (!cond)
    {
        std::cout << "[FAIL] " << msg << std::endl;
        g_failures++;
    }
    else
    {
        std::cout << "[OK]   " << msg << std::endl;
    }
}

int main()
{
    HttpRequest req;

    // Test 1: método y versión
    req.setMethod("GET");
    req.setVersion("HTTP/1.1");
    assertTrue(req.getMethod() == HTTP_METHOD_GET, "Method GET -> enum");
    assertTrue(req.getVersion() == HTTP_VERSION_1_1, "Version HTTP/1.1 -> enum");

    // Test 2: headers case-insensitive
    // TODO: addHeaders now expects lowercase keys; update these to "host"/"connection".
    req.addHeaders("Host", "example.com");
    assertTrue(req.getHeader("host") == "example.com", "Header Host (lower)");
    assertTrue(req.getHeader("HOST") == "example.com", "Header Host (upper)");

    // Test 3: path y query
    req.setPath("/index.html");
    req.setQuery("page=1&sort=asc");
    assertTrue(req.getPath() == "/index.html", "Path set/get");
    assertTrue(req.getQuery() == "page=1&sort=asc", "Query set/get");

    // Test 4: body binario simple
    std::string chunk;
    chunk.push_back('A');
    chunk.push_back('\0');
    chunk.push_back('B');
    req.addBody(chunk.begin(), chunk.end());
    std::vector<char> body = req.getBody();
    assertTrue(body.size() == 3, "Body size (incluye null)");
    assertTrue(body[0] == 'A' && body[1] == '\0' && body[2] == 'B', "Body content");

    // Test 5: shouldCloseConnection()
    req.setVersion("HTTP/1.1");
    // TODO: addHeaders now expects lowercase keys; update to "connection".
    req.addHeaders("Connection", "close");
    assertTrue(req.shouldCloseConnection(), "HTTP/1.1 + Connection: close -> cerrar");

    req.clear();
    req.setVersion("HTTP/1.1");
    assertTrue(!req.shouldCloseConnection(), "HTTP/1.1 sin Connection -> keep-alive");

    req.clear();
    req.setVersion("HTTP/1.0");
    assertTrue(req.shouldCloseConnection(), "HTTP/1.0 sin Connection -> cerrar");

    req.clear();
    req.setVersion("HTTP/1.0");
    // TODO: addHeaders now expects lowercase keys; update to "connection".
    req.addHeaders("Connection", "keep-alive");
    assertTrue(!req.shouldCloseConnection(), "HTTP/1.0 + keep-alive -> mantener");

    if (g_failures == 0)
        std::cout << "\nTodos los tests pasaron." << std::endl;
    else
        std::cout << "\nTests fallidos: " << g_failures << std::endl;

    return g_failures == 0 ? 0 : 1;
}

