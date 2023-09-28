project(h5pp_lib)


find_package(HDF5 COMPONENTS C CXX HL REQUIRED)

add_library(h5pp archive.cpp object.cpp)
target_link_libraries(h5pp PUBLIC HDF5::HDF5 hdf5::hdf5_hl)