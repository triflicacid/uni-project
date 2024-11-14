cmake_minimum_required(VERSION 3.6)
project(visualiser LANGUAGES CXX)

include_directories(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(visualiser
        ../shared/util.cpp ../shared/messages/message.cpp ../shared/messages/list.cpp
        src/assembly.cpp
        main.cpp)
