cmake_minimum_required(VERSION 3.9)

project(expression)
add_executable(expression expression.cpp expression.h)
target_compile_features(expression PRIVATE cxx_std_17)
