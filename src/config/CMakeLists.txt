add_library(config STATIC
        ConfigParser.cpp
        ConfigException.cpp
        ServerConfig.cpp
        ConfigUtils.cpp
        LocationConfig.cpp
        ConfigParser.hpp
        ConfigException.hpp
        ServerConfig.hpp
        ConfigUtils.hpp
        LocationConfig.hpp
)

target_include_directories(config PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR} # src/config/
)

target_link_libraries(config PRIVATE
        common
)
