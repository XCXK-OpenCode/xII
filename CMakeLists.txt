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

# Item::Text 使用 FreeType 做字形栅格化 / 轮廓提取。
# 优先使用 libs/freetype 子模块（自包含，不依赖系统 freetype 开发包），
# 子模块未检出时才回退到系统的 find_package。
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/freetype/CMakeLists.txt")
    message(STATUS "Using bundled FreeType: libs/freetype")

    # 这些都是可选依赖，找不到时会自动跳过；显式关闭可让构建结果与系统环境无关。
    set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_ZLIB FALSE CACHE BOOL "" FORCE)
    set(FT_ENABLE_ERROR_STRINGS TRUE CACHE BOOL "" FORCE)

    add_subdirectory(libs/freetype EXCLUDE_FROM_ALL)

    # 上游只对安装导出使用 Freetype::Freetype 这个名字，构建树里需要自己建别名，
    # 这样下面的 target_link_libraries 对两种来源写法一致。
    if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype-interface)
    endif()
else()
    message(STATUS "Bundled FreeType not found, falling back to system find_package")
    find_package(Freetype REQUIRED)
endif()

add_library(xII STATIC
    src/Camera/Outline.cpp
    src/Camera/Standard.cpp
    src/Item/Base.cpp
    src/Item/General.cpp
    src/Item/Image.cpp
    src/Item/Text.cpp
    src/Item/Renderer/Rainbow.cpp
    src/Item/Renderer/Line.cpp
    src/Item/Renderer/Fill.cpp
    src/Item/Renderer/Curves.cpp
    src/Item/Shape/Rectangle.cpp
    src/Screen/RGBA8888.cpp
    src/Screen/PointPos.cpp
)

# GPU 渲染后端需要 SDL 的 render API；PUBLIC 让使用方也拿到头文件路径
# FreeType 的头文件路径也需要传递给包含 Text.h 的使用方
# （Text.h 本身只前向声明 FT_Face，但链接必须带上 freetype）
target_link_libraries(xII PUBLIC SDL3::SDL3-shared Freetype::Freetype)

add_subdirectory(examples/outline_camera/)
add_subdirectory(examples/line_renderer/)
add_subdirectory(examples/fill_renderer/)
add_subdirectory(examples/rainbow_renderer/)
add_subdirectory(examples/fadein_renderer/)

add_subdirectory(test/20261011-2209)
add_subdirectory(test/20261018-173742)
add_subdirectory(test/20261021-190336)
add_subdirectory(test/20261021-193500)
add_subdirectory(test/20261021-195303)
add_subdirectory(test/20261021-210918)
add_subdirectory(test/20261021-213739)
add_subdirectory(test/20261020-200709)
add_subdirectory(test/20261020_202547)
