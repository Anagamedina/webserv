add_library(http STATIC
    HttpParser.cpp
    HttpParserBody.cpp
    HttpParserHeaders.cpp
    HttpParserStartLine.cpp
    HttpRequest.cpp
    HttpResponse.cpp
    HttpHeaderUtils.cpp
    HttpParser.hpp
    HttpRequest.hpp
    HttpResponse.hpp
    HttpHeaderUtils.hpp
)

target_include_directories(http PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(http PRIVATE
    common
)
