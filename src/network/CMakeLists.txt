add_library(network STATIC
    EpollWrapper.cpp
    ServerManager.cpp
    TcpListener.cpp
    EpollWrapper.hpp
    ServerManager.hpp
    TcpListener.hpp
)

target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network PRIVATE
    common
    config
    client
)
