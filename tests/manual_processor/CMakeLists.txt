set(HTTP_INC_DIR ${CMAKE_SOURCE_DIR}/src/http)
set(CLIENT_INC_DIR ${CMAKE_SOURCE_DIR}/src/client)

add_executable(manual_request_processor
        manual_request_processor.cpp
        ../../src/client/ErrorUtils.cpp
        ../../src/client/ResponseUtils.cpp
        ../../src/client/AutoindexRenderer.cpp
        ../../src/client/StaticPathHandler.cpp
        ../../src/client/RequestProcessorUtils.cpp
        ../../src/client/RequestProcessor.cpp
        ../../src/http/HttpHeaderUtils.cpp
        ../../src/http/HttpRequest.cpp
        ../../src/http/HttpResponse.cpp
)

target_include_directories(manual_request_processor PRIVATE
        ${HTTP_INC_DIR}
        ${CLIENT_INC_DIR}
)
