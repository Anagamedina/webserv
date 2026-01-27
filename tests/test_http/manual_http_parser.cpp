#include <iostream>
#include <string>
#include "../../src/http/HttpParser.hpp"

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
    // Caso 1: Request completa en una sola cadena
    {
        HttpParser parser;
        parser.consume("GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n");
        assertTrue(parser.getState() == COMPLETE, "Request completa -> COMPLETE");

        const HttpRequest& req = parser.getRequest();
        // DEBUG
        // std::cout << "Method=" << req.getMethod()
        //           << " Version=" << req.getVersion()
        //           << " Path=" << req.getPath()
        //           << " Query=" << req.getQuery() << std::endl;
        assertTrue(req.getMethod() == HTTP_METHOD_GET, "Method GET");
        assertTrue(req.getVersion() == HTTP_VERSION_1_1, "Version HTTP/1.1");
        assertTrue(req.getPath() == "/index.html", "Path /index.html");
        assertTrue(req.getQuery().empty(), "Query vacía");
        assertTrue(req.getHeader("Host") == "example.com", "Header Host");
    }

    // Caso 2: Request fragmentada en varias llamadas
    {
        HttpParser parser;
        parser.consume("GET / HTTP/1.1\r\nHo");
        assertTrue(parser.getState() == PARSING_HEADERS || parser.getState() == PARSING_START_LINE,
                   "Fragmento 1 -> no completa");

        parser.consume("st: a.com\r\n\r\n");
        assertTrue(parser.getState() == COMPLETE, "Fragmento 2 -> COMPLETE");
        // DEBUG:
        // std::cout << "Host fragmentado=" << parser.getRequest().getHeader("Host") << std::endl;
        assertTrue(parser.getRequest().getHeader("Host") == "a.com", "Header Host fragmentado");
    }

    // Caso 3: Request con Content-Length y body
    {
        HttpParser parser;
        parser.consume("POST /upload HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nHello");
        assertTrue(parser.getState() == COMPLETE, "POST con body -> COMPLETE");
        // DEBUG:
        // std::cout << "Body size=" << parser.getRequest().getBody().size() << std::endl;
        assertTrue(parser.getRequest().getMethod() == HTTP_METHOD_POST, "Method POST");
        assertTrue(parser.getRequest().getBody().size() == 5, "Body size 5");
    }

    // Caso 3b: Content-Length fragmentado en dos llamadas
    {
        HttpParser parser;
        parser.consume("POST /p HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nHe");
        assertTrue(parser.getState() == PARSING_BODY, "Body parcial -> PARSING_BODY");

        parser.consume("llo");
        assertTrue(parser.getState() == COMPLETE, "Body completo -> COMPLETE");
        // DEBUG:
        // std::cout << "Body size (fragmentado)=" << parser.getRequest().getBody().size() << std::endl;
        assertTrue(parser.getRequest().getBody().size() == 5, "Body size 5 (fragmentado)");
    }

    // Caso 4: Request chunked
    {
        HttpParser parser;
        parser.consume("POST /c HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n");
        parser.consume("5\r\nHello\r\n0\r\n\r\n");
        assertTrue(parser.getState() == COMPLETE, "Chunked -> COMPLETE");
        // DEBUG:
        // std::cout << "Chunked body size=" << parser.getRequest().getBody().size() << std::endl;
        assertTrue(parser.getRequest().getBody().size() == 5, "Chunked body size 5");
    }

    // Caso 4b: Chunked fragmentado en varias llamadas
    {
        HttpParser parser;
        parser.consume("POST /c HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n");
        parser.consume("5\r\nHe");
        assertTrue(parser.getState() == PARSING_BODY, "Chunked parcial -> PARSING_BODY");

        parser.consume("llo\r\n0\r\n\r\n");
        assertTrue(parser.getState() == COMPLETE, "Chunked completo -> COMPLETE");
        // DEBUG:
        // std::cout << "Chunked body size (fragmentado)=" << parser.getRequest().getBody().size() << std::endl;
        assertTrue(parser.getRequest().getBody().size() == 5, "Chunked body size 5 (fragmentado)");
    }

    // Caso 5: Request mal formada (header sin ':')
    {
        HttpParser parser;
        parser.consume("GET / HTTP/1.1\r\nHost example.com\r\n\r\n");
        assertTrue(parser.getState() == ERROR, "Header sin ':' -> ERROR");
    }

    if (g_failures == 0)
        std::cout << "\nTodos los tests pasaron." << std::endl;
    else
        std::cout << "\nTests fallidos: " << g_failures << std::endl;

    return g_failures == 0 ? 0 : 1;
}

