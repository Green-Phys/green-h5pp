project(h5pp_lib)

find_package(HDF5 COMPONENTS C CXX HL REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(h5pp archive.cpp common.cpp)
target_link_libraries(h5pp PUBLIC HDF5::HDF5 hdf5::hdf5_hl)
target_include_directories(h5pp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
