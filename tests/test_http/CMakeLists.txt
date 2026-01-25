# No necesitas set(CMAKE_CXX_STANDARD) otra vez, ya viene del principal

# Definimos dónde están los headers del parser para no repetir rutas largas
set(HTTP_INC_DIR ${CMAKE_SOURCE_DIR}/src/http)

add_executable(test_http_parser
        manual_http_parser.cpp           # Este archivo debe estar en tests/test_http/
        ../../src/http/HttpParser.cpp
        ../../src/http/HttpParserStartLine.cpp
        ../../src/http/HttpParserHeaders.cpp
        ../../src/http/HttpParserBody.cpp
        ../../src/http/HttpRequest.cpp
)

# Esto es vital para que manual_http_parser.cpp encuentre "HttpParser.hpp"
target_include_directories(test_http_parser PRIVATE
        ${HTTP_INC_DIR}
)


add_executable(test_http_request
        manual_http_request.cpp           # Este archivo debe estar en tests/test_http/
        ../../src/http/HttpRequest.cpp
)

# Esto es vital para que manual_http_parser.cpp encuentre "HttpParser.hpp"
target_include_directories(test_http_request PRIVATE
        ${HTTP_INC_DIR}
)
