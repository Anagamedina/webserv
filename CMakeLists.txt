cmake_minimum_required(VERSION 4.0)
project(webserv)

set(CMAKE_CXX_STANDARD 98)

# Source files
set(SOURCES
	src/main.cpp
	src/network/EpollWrapper.cpp
	src/network/TcpListener.cpp
	src/network/ServerManager.cpp
	src/http/HttpParser.cpp
	src/http/HttpParserBody.cpp
	src/http/HttpParserHeaders.cpp
	src/http/HttpParserStartLine.cpp
	src/http/HttpRequest.cpp
)

add_executable(webserv ${SOURCES})
target_include_directories(webserv PRIVATE ${CMAKE_SOURCE_DIR}/src)

# ---- HTTP tests (manual) ----
add_executable(test_http_request
    tests/manual_http_request.cpp
    src/http/HttpRequest.cpp
)
target_include_directories(test_http_request PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(test_http_parser
    tests/manual_http_parser.cpp
    src/http/HttpParser.cpp
    src/http/HttpParserStartLine.cpp
    src/http/HttpParserHeaders.cpp
    src/http/HttpParserBody.cpp
    src/http/HttpRequest.cpp
)
target_include_directories(test_http_parser PRIVATE ${CMAKE_SOURCE_DIR}/src)