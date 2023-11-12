project(h5pp_lib)

find_package(HDF5 COMPONENTS C HL REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(h5pp archive.cpp common.cpp)
if(${CMAKE_VERSION} VERSION_LESS "3.20.0") 
    message("Please consider to switch to CMake 3.20.0")
    target_link_libraries(h5pp PUBLIC ${HDF5_C_LIBRARIES} ${HDF5_C_HL_LIBRARIES})
  target_include_directories(h5pp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${HDF5_C_INCLUDE_DIRS})
else()
  target_link_libraries(h5pp PUBLIC hdf5::hdf5 hdf5::hdf5_hl)
  target_include_directories(h5pp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

