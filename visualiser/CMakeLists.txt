cmake_minimum_required(VERSION 3.6)
project(visualiser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
        GIT_TAG v5.1.0
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ftxui)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(visualiser
        ../shared/constants.cpp ../shared/util.cpp ../shared/messages/message.cpp ../shared/messages/list.cpp
        ../processor/src/bus.cpp ../processor/src/core.cpp ../processor/src/cpu.cpp ../processor/src/debug.cpp ../processor/src/dram.cpp
        src/data.cpp src/processor.cpp src/style.cpp
        src/components/checkbox.cpp src/components/custom_dropdown.cpp src/components/Scroller.cpp
        src/tabs/execution.cpp src/tabs/memory.cpp src/tabs/registers.cpp src/tabs/settings.cpp
        src/screen.cpp
        main.cpp)

target_include_directories(visualiser PRIVATE src)
target_link_libraries(visualiser
        PRIVATE ftxui::screen
        PRIVATE ftxui::dom
        PRIVATE ftxui::component
)
