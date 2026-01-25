cmake_minimum_required(VERSION 3.10) # 4.0 es muy alto, 3.10 es estándar
project(webserv)

set(CMAKE_CXX_STANDARD 98)
add_compile_options(-Wall -Wextra -Werror)

# Definimos la ruta raíz para que los sub-archivos la conozcan
set(WEBSERV_ROOT ${CMAKE_SOURCE_DIR})

# Si quieres que el binario de los tests se cree,
# tenemos que incluir la carpeta donde está tu código
#add_subdirectory(src/http)
add_subdirectory(tests/test_http)


