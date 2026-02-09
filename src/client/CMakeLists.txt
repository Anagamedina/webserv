add_library(client STATIC
        AutoindexRenderer.cpp
        Client.cpp
        ClientCgi.cpp
        ErrorUtils.cpp
        RequestProcessor.cpp
        RequestProcessorUtils.cpp
        ResponseUtils.cpp
        StaticPathHandler.cpp
        AutoindexRenderer.hpp
        Client.hpp
        ErrorUtils.hpp
        RequestProcessor.hpp
        RequestProcessorUtils.hpp
        ResponseUtils.hpp
        StaticPathHandler.hpp
)

target_include_directories(client PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR} # src/client/
)

target_link_libraries(client PRIVATE
        cgi
        config
        http
        common
)
