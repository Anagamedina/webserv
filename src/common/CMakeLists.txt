# Header-only library

# INTERFACE library: no se compila, solo proporciona headers
# Útil para bibliotecas que solo contienen .hpp sin .cpp
add_library(common INTERFACE)

# Especificamos donde están los headers para que otros módulos puedan incluirlos
# INTERFACE: solo quien use 'common' puede acceder a estos directorios
# Esto permite hacer: #include "common/namespaces.hpp"
target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR} # src/common/
)

# Nota: No necesitamos target_sources() porque INTERFACE libraries
# no tienen archivos fuente para compilar, solo headers para incluir.
