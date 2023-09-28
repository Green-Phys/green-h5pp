cmake_minimum_required(VERSION 3.22)
project(h5pp)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
add_subdirectory(src)
add_subdirectory(test)