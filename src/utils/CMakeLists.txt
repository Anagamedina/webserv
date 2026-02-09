add_library(utils STATIC
        StringUtils.cpp
        StringUtils.hpp
        StringUtils.tpp
)

target_include_directories(utils PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
