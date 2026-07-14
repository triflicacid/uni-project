cmake_minimum_required(VERSION 3.6)
project(all)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
include_directories(.)
include_directories(shared)

add_subdirectory(processor)
add_subdirectory(assembler)
add_subdirectory(compiler)
add_subdirectory(visualiser)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating HTML documentation with Doxygen"
        VERBATIM)
endif()
