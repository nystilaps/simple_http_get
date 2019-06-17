cmake_minimum_required (VERSION 2.6)

project (http_get)

set (CMAKE_CXX_STANDARD 11)

add_compile_options ( -Wall -Wextra )

add_executable(http_get main.cpp Socket.cpp http_util.cpp)



