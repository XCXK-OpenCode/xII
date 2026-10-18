cmake_minimum_required(VERSION 4.3)
project(xII)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/SDL EXCLUDE_FROM_ALL)

include_directories(src)

add_library(xII STATIC
    src/Camera/Outline.cpp
    src/Camera/Standard.cpp
    src/Item/Base.cpp
    src/Item/General.cpp
    src/Item/Renderer/Rainbow.cpp
    src/Item/Renderer/Line.cpp
    src/Item/Renderer/Fill.cpp
    src/Item/Renderer/Curves.cpp
    src/Item/Shape/Rectangle.cpp
    src/Screen/RGBA8888.cpp
    src/Screen/PointPos.cpp
)

add_subdirectory(examples/outline_camera/)
add_subdirectory(examples/line_renderer)

add_subdirectory(test/20261011-2209)