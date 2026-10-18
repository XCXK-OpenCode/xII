cmake_minimum_required(VERSION 3.22)
project(example_outline_camera LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(example_outline_camera main.cpp)

target_link_libraries(example_outline_camera PRIVATE SDL3::SDL3-shared xII)