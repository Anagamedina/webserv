cmake_minimum_required(VERSION 3.10)

project(webserv
    VERSION 0.1.0
    DESCRIPTION "HTTP Web Server"
    LANGUAGES CXX
)

# ---------- global configuration ---------------------------------------------
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Werror)

# ---------- mode selection ---------------------------------------------------
option(USE_MANUAL "Use manually specified source files" OFF)

if(USE_MANUAL)
    message(STATUS "MANUAL mode – using predefined source list")

    set(WEBSERV_SOURCES
        src/main.cpp
        src/network/EpollWrapper.cpp
        src/network/TcpListener.cpp
        src/network/ServerManager.cpp
        # config sources are NOT listed here (built as a library)
    )

else()
    message(STATUS "AUTO mode – discovering sources under src/")

    file(GLOB_RECURSE WEBSERV_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
    )

    # Include all sources found
    # list(FILTER WEBSERV_SOURCES EXCLUDE REGEX ".*/src/config/.*")
endif()

# ---------- config library (optional) ---------------------------------------
set(CONFIG_DIR "${CMAKE_SOURCE_DIR}/src/config")

if(EXISTS "${CONFIG_DIR}/CMakeLists.txt")
    message(STATUS "Config library detected, enabling build")
    add_subdirectory(${CONFIG_DIR})
    set(CONFIG_ENABLED ON)
else()
    message(STATUS "Config library not found, skipping build")
    set(CONFIG_ENABLED OFF)
endif()

# ---------- main executable --------------------------------------------------
add_executable(webserv ${WEBSERV_SOURCES})

if(CONFIG_ENABLED)
    target_link_libraries(webserv PRIVATE config)
endif()

# ---------- include directories ---------------------------------------------
target_include_directories(webserv PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/cgi
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/http
    ${CMAKE_SOURCE_DIR}/src/network
    ${CMAKE_SOURCE_DIR}/src/utils
)

# ---------- testing ----------------------------------------------------------
option(BUILD_TESTING "Build the testing tree." OFF)

if(BUILD_TESTING AND EXISTS "${CMAKE_SOURCE_DIR}/tests/unit")
    enable_testing()
    include_directories(${CMAKE_SOURCE_DIR}/lib)
    add_subdirectory(tests/unit)
    add_executable(Testing tests/unit/main_test.cpp)

    if(CONFIG_ENABLED)
        target_link_libraries(Testing PRIVATE config)
    endif()
endif()
