cmake_minimum_required(VERSION 4.3)
project(xII)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# 默认使用 Release：未指定构建类型时 CMake 不加任何优化（-O0），
# 逐像素的 Fill/Line 渲染会慢 2~3 倍。显式传入 -DCMAKE_BUILD_TYPE=Debug 仍可覆盖。
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE defaulted to Release")
endif()

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/SDL EXCLUDE_FROM_ALL)

include_directories(src)

# ---------------------------------------------------------------------------
# 渲染后端：CPU 光栅化永远编译；OpenGL 后端按需编译。
#   cmake -DXII_ENABLE_OPENGL=ON ..
# 会定义 XII_ENABLE_OPENGL=1，并编译 Screen/GLFunctions.cpp / Screen/GLCanvas.cpp。
# ---------------------------------------------------------------------------
option(XII_ENABLE_OPENGL "Compile the OpenGL 3.3 GPU backend" OFF)

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

if(XII_ENABLE_OPENGL)
    target_sources(xII PRIVATE
        src/Screen/GLFunctions.cpp
        src/Screen/GLCanvas.cpp
    )
    target_compile_definitions(xII PUBLIC XII_ENABLE_OPENGL=1)
    message(STATUS "xII: OpenGL GPU backend ENABLED")
else()
    target_compile_definitions(xII PUBLIC XII_ENABLE_OPENGL=0)
    message(STATUS "xII: CPU rasterizer only (pass -DXII_ENABLE_OPENGL=ON for GPU)")
endif()

# GPU 渲染后端需要 SDL 的 render/GL API；PUBLIC 让使用方也拿到头文件路径
target_link_libraries(xII PUBLIC SDL3::SDL3-shared)

add_subdirectory(examples/outline_camera/)
add_subdirectory(examples/line_renderer/)
add_subdirectory(examples/fill_renderer/)
add_subdirectory(examples/rainbow_renderer/)
add_subdirectory(examples/fadein_renderer/)
add_subdirectory(examples/gl_renderer/)

add_subdirectory(test/20261011-2209)
add_subdirectory(test/20261018-173742)