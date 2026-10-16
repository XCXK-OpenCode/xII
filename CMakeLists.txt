cmake_minimum_required(VERSION 4.3)
project(xII)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include_directories(src)

add_library(xII STATIC
    src/Camera/Outline.cpp
    src/Camera/Standard.cpp
    src/Item/Base.cpp
    src/Item/General.cpp
    src/Item/Renderer/Rainbow.cpp
    src/Item/Renderer/Line.cpp
    src/Item/Renderer/Fill.cpp
    src/Item/Shape/Rectangle.cpp
    src/Screen/RGBA8888.cpp
    src/Screen/PointPos.cpp
)
