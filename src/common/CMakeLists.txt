# Header-only library

# STATIC library: compila los archivos .cpp en un archivo .a
add_library(common STATIC
    StringUtils.cpp
    StringUtils.hpp
    StringUtils.tpp
    namespaces.hpp
)

# Especificamos donde están los headers para que otros módulos puedan incluirlos
# PUBLIC: tanto 'common' como quien use 'common' accederán a estos directorios
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} # src/common/
)

# Nota: No necesitamos target_sources() porque INTERFACE libraries
# no tienen archivos fuente para compilar, solo headers para incluir.
