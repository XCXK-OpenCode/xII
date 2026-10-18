cmake_minimum_required(VERSION 3.22)
project(example_gl_renderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(example_gl_renderer main.cpp)

target_link_libraries(example_gl_renderer PRIVATE SDL3::SDL3-shared xII)
