[![GitHub license](https://img.shields.io/github/license/Green-Phys/green-h5pp?cacheSeconds=3600&color=informational&label=License)](./LICENSE)
[![GitHub license](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/compiler_support/17)

![h5pp](https://github.com/Green-Phys/green-h5pp/actions/workflows/h5pp-test.yaml/badge.svg)

# Green-h5pp

Light implementation for common routines to access hdf5 files.
Interface uses same representation of complex numbers as in h5py. This library is a part of 
Green quantum many-body framework.

## Basic usage

`green-h5pp` is a C++ library that provides basic routines to operate with HDF5 data.
It uses `operator <<` and `operator >>` to write and read data. It will automatically 
create dataset and all the whole groups tree this dataset belongs to.

To add this library into your project, first 

```CMake
Include(FetchContent)

FetchContent_Declare(
        green-h5pp
        GIT_REPOSITORY https://github.com/Green-Phys/green-h5pp.git
        GIT_TAG origin/main # or a later release
)
FetchContent_MakeAvailable(green-h5pp)
```
Add predefined alias `GREEN::H5PP` it to your target:
```CMake
target_link_libraries(<target> PUBLIC GREEN::H5PP)
```
And then simply include the following header:
```cpp
#include <green/h5pp/archive.h>
```

***
Here is a small example of some possible operations:


```cpp
using namespace green::h5pp;

// Open file for write (file will be purged if previously available)
archive ar("filename", "w")

// Create group 'test' and dataset 'data' and write scalar into it
ar["test/data"] << 10.0;

// If datatype has method 'shape()' it will be considered as multidimensional data.
// If datatype has method 'size()' it will be cosidered as a one-dimensional data.

// Here we create vector with 100 elements and write it into dataset 'vector'
std::vector<double> vector(100);
ar["test/vector"] << vector;

// For std::complex types it will create a composite type for <real, imag> pairs

std::complex<double> complex;
ar["test/complex"] << complex;
```

# Acknowledgements

This work is supported by National Science Foundation under the award CSSI-2310582
