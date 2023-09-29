cmake_minimum_required(VERSION 3.22)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
project(h5pp)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(FATAL_ERROR "In-source builds are not allowed")
endif()

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
add_subdirectory(src)

option(Build_Tests "Build tests" ON)
if (Build_Tests)
    enable_testing()
    add_subdirectory(test)
endif ()
