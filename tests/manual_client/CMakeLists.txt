set(HTTP_INC_DIR ${CMAKE_SOURCE_DIR}/src/http)
set(CLIENT_INC_DIR ${CMAKE_SOURCE_DIR}/src/client)

add_executable(manual_client
        manual_client.cpp
        ../../src/client/Client.cpp
        ../../src/client/ClientCgi.cpp
        ../../src/client/ErrorUtils.cpp
        ../../src/client/ResponseUtils.cpp
        ../../src/client/AutoindexRenderer.cpp
        ../../src/client/StaticPathHandler.cpp
        ../../src/client/RequestProcessorUtils.cpp
        ../../src/client/RequestProcessor.cpp
        ../../src/http/HttpHeaderUtils.cpp
        ../../src/http/HttpParser.cpp
        ../../src/http/HttpParserStartLine.cpp
        ../../src/http/HttpParserHeaders.cpp
        ../../src/http/HttpParserBody.cpp
        ../../src/http/HttpRequest.cpp
        ../../src/http/HttpResponse.cpp
        ../../src/network/ServerManager.cpp
        ../../src/cgi/CgiExecutor.cpp
)

target_include_directories(manual_client PRIVATE
        ${HTTP_INC_DIR}
        ${CLIENT_INC_DIR}
)
