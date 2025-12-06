cmake_minimum_required(VERSION 3.15)
project(MyProject LANGUAGES CXX)

# --- Options ---
option(ENABLE_ASAN "Enable AddressSanitizer (ASan)" OFF)
option(ENABLE_LSAN "Enable LeakSanitizer (LSan)" OFF)
option(GLOB_SOURCES "Automatically find all .cpp files" ON)  # New: Auto-find sources

# --- Sanitizer Conflict Check ---
if(ENABLE_ASAN AND ENABLE_LSAN)
    message(FATAL_ERROR "Cannot enable ASan and LSan at the same time")
endif()

# --- Compiler Settings ---
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -pedantic -Wshadow")

# --- Source Files ---
if(GLOB_SOURCES)
    # Auto-find all .cpp files recursively
    file(GLOB_RECURSE SOURCES "src/*.cpp")
    message(STATUS "Using GLOB for sources: ${SOURCES}")
else()
    # Manual listing (fallback)
    set(SOURCES
        src/main.cpp
    )
endif()

# --- Executable ---
add_executable(ex ${SOURCES})

# --- Include Directories ---
# Auto-find all header directories (recursively under src/ and include/)
file(GLOB_RECURSE HEADER_DIRS LIST_DIRECTORIES true "src/*.hpp" "include/*.hpp")
list(REMOVE_DUPLICATES HEADER_DIRS)  # Remove duplicates
foreach(dir IN LISTS HEADER_DIRS)
    get_filename_component(PARENT_DIR ${dir} DIRECTORY)
    target_include_directories(ex PRIVATE ${PARENT_DIR})
endforeach()

# --- Sanitizers ---
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer (ASan) enabled")
    target_compile_options(ex PRIVATE -g -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(ex PRIVATE -fsanitize=address)
endif()

if(ENABLE_LSAN)
    message(STATUS "LeakSanitizer (LSan) enabled")
    target_compile_options(ex PRIVATE -g -fsanitize=leak)
    target_link_options(ex PRIVATE -fsanitize=leak)
endif()

# --- GTest ---
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
    )
    # Prevent GoogleTest from forcing C++11+
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(CMAKE_CXX_STANDARD 98)

    FetchContent_MakeAvailable(googletest)

    enable_testing()

    # Define test sources
    set(TEST_SOURCES
        tests/test_scalar_converter.cpp
		src/scalar_converter/ScalarConverter.cpp
    )

    add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests PRIVATE GTest::gtest_main)

    # Inherit include dirs from main project (so tests see headers)
    target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

    include(GoogleTest)
    gtest_discover_tests(tests)
endif()

# --- Clean Target ---
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Removing all build files..."
)
