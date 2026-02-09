add_library(cgi STATIC
        CgiExecutor.cpp
        CgiProcess.cpp
        CgiExecutor.hpp
        CgiProcess.hpp
)

target_include_directories(cgi PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR} # src/cgi
)

# Enlazamos con las librerías necesarias
# PRIVATE porque son dependencias internas de la implementación de cgi
target_link_libraries(cgi PRIVATE
        config
        http
        common
)
