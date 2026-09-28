cmake_minimum_required(VERSION 3.24)

project(
    ascii_universe
    VERSION 0.4.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ==================================================
# EnTT
# ==================================================

FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(EnTT)

# ==================================================
# nlohmann/json
# ==================================================

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(json)

# ==================================================
# SDL3
# ==================================================

find_package(
    SDL3
    REQUIRED
    CONFIG
    COMPONENTS SDL3
)

# ==================================================
# Executable
# ==================================================

add_executable(
    ascii_universe

    src/main.cpp

    src/Simulation.cpp
    src/PlayerController.cpp
    src/SDLFrontend.cpp
    src/WorldGenerator.cpp
    src/SaveManager.cpp

    src/systems/DesignationSystems.cpp
    src/systems/MovementSystem.cpp
    src/systems/ExcavationSystems.cpp
    src/systems/FellingSystems.cpp
    src/systems/StockpileSystems.cpp
    src/systems/ItemSystems.cpp
    src/systems/HaulingSystems.cpp
    src/systems/WaterSystem.cpp
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
        SDL3::SDL3
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
