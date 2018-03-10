cmake_minimum_required(VERSION 3.9)

project(algebra)

add_library(algebra
    src/expression.cpp
    src/expression.h
    src/parser.cpp
    src/parser.h
    src/ptr.h
)

target_compile_features(algebra PUBLIC cxx_std_17)
target_include_directories(algebra
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

add_executable(simplify
    test/simplify.cpp
)

target_link_libraries(simplify PRIVATE algebra)
