cmake_minimum_required(VERSION 3.24)

project(
    ascii_universe
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# --------------------------------------------------
# EnTT
# --------------------------------------------------

FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(EnTT)

# --------------------------------------------------
# nlohmann/json
# --------------------------------------------------

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(json)

# --------------------------------------------------
# Engine
# --------------------------------------------------

add_executable(
    ascii_universe

    src/main.cpp
)

target_include_directories(
    ascii_universe
    PRIVATE
        include
)

target_link_libraries(
    ascii_universe
    PRIVATE
        EnTT::EnTT
        nlohmann_json::nlohmann_json
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
        ascii_universe
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()
