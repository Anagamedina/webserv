cmake_minimum_required(VERSION 3.10)
project(webserv
        VERSION 0.1.0
        DESCRIPTION "HTTP Web Server"
        LANGUAGES CXX
)

# Global Configuration
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_compile_options(-Wall -Wextra -Werror)

#include subdirectories of each module in src/
add_subdirectory(src/config)

# main executable
#        src/main.cpp
add_executable(webserv
        src/config/mainConfig.cpp
)

# enlazar con libreria.
target_link_libraries(webserv PRIVATE
        config
)

