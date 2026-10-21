cmake_minimum_required(VERSION 3.22)
project(test_20261021_195303)

set(CMAKE_CXX_STANDARD 20)

add_executable(test_20261021_195303 main.cpp)

target_link_libraries(test_20261021_195303 SDL3::SDL3-shared xII)
