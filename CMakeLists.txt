cmake_minimum_required(VERSION 3.6)
project(all)

set(CMAKE_CXX_STANDARD 20)

unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
include_directories(.)
include_directories(shared)

add_subdirectory(processor)
add_subdirectory(assembler)
add_subdirectory(compiler)
add_subdirectory(visualiser)
