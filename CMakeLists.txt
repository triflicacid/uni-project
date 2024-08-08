cmake_minimum_required(VERSION 3.28)
project(all)

set(CMAKE_CXX_STANDARD 20)

unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
include_directories(".")

add_subdirectory("processor" "processor/out")
