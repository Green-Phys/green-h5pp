project(h5pp_test)

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.3 # or a later release
)

FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_executable(h5_test h5_archive_test.cpp h5_dataset_test.cpp utils_test.cpp
        h5_common_test.cpp)
target_compile_definitions(h5_test PRIVATE TEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(h5_test
        PRIVATE
        Catch2::Catch2WithMain
        GREEN::H5PP)

include(CTest)
include(Catch)
catch_discover_tests(h5_test)
