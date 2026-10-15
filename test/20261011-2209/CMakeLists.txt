cmake_minimum_required(VERSION 3.22)
project(test)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(/home/qpwser/Projects/xII ${CMAKE_BINARY_DIR}/xII)
include_directories(/home/qpwser/Projects/xII/src)

add_subdirectory(lib/SDL)
include_directories(lib/SDL/include)

add_executable(test main.cpp)

target_link_libraries(test SDL3::SDL3-shared xII)
