cmake_minimum_required(VERSION 4.0)
project(webserv
    VERSION 0.1.0
        DESCRIPTION ""
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_compile_options(-Wall -Wextra -Werror)

#add_subdirectory(src)

add_executable(webserv
        src/main.cpp
)
add_executable(test-socket docs/allowed_functions/TestServer.cpp)
