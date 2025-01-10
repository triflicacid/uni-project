cmake_minimum_required(VERSION 3.6)
project(language LANGUAGES CXX)

include_directories("src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
add_executable(language ../shared/util.cpp ../shared/messages/message.cpp ../shared/messages/MessageWithSource.cpp ../shared/messages/list.cpp
        main.cpp)
