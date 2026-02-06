# No necesita project() aquí
set(CMAKE_CXX_STANDARD 11) # use when testing

# get all files inside this directory
#file(GLOB TEST_SOURCES "*.cpp")
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Executable
add_executable(unit_tests ${TEST_SOURCES}
        #        config/test_config_parser.cpp
)

# change version to c++11
set_target_properties(unit_tests PROPERTIES CXX_STANDARD 11)

# includes necesario (para encontrar catch2 y los headers del proyecto)
# Link against the config library!
target_link_libraries(unit_tests PRIVATE
        config
)

# Includes needed
target_include_directories(unit_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/config
        ${CMAKE_SOURCE_DIR}/lib/catch2
)
