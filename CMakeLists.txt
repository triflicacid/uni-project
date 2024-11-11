cmake_minimum_required(VERSION 3.6)
project(all)

set(CMAKE_CXX_STANDARD 20)

unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
include_directories(.)
include_directories(util)

add_subdirectory(processor)
add_subdirectory(assembler)
add_subdirectory(language)
add_subdirectory(visualiser)
