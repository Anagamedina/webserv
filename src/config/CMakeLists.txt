add_library(config STATIC
        ConfigParser.cpp
        ConfigException.cpp
        ServerConfig.cpp
        ConfigUtils.cpp
        ConfigUtils.hpp
        LocationConfig.cpp
)

# definimos donde estan los headers (.hpp)

# Especificamos directorios de include para esta biblioteca
# PUBLIC: tanto config como quien use config puede acceder
# PRIVATE: solo config puede acceder
# INTERFACE: solo quien use config puede acceder
target_include_directories(config PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}  # src/config/
)

# Si config necesita otras bibliotecas, las enlazarías aquí:
#target_link_libraries(config PRIVATE otra-lib)
