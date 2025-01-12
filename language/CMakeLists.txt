cmake_minimum_required(VERSION 3.6)
project(language LANGUAGES CXX)

include_directories("src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(language ../shared/util.cpp ../shared/messages/message.cpp ../shared/messages/MessageWithSource.cpp ../shared/messages/list.cpp ../shared/istream_wrapper.cpp
        src/config.cpp
        src/lexer.cpp src/parser.cpp
        main.cpp)
