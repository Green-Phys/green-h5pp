project(h5pp_test)

include(FetchContent)

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(h5_test h5_archive_test.cpp h5_dataset_test.cpp utils_test.cpp)

target_compile_definitions(h5_test PRIVATE TEST_PATH= "${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(h5_test
        PRIVATE
        Catch2::Catch2WithMain
        h5pp)

add_test(h5_catchtest h5_test)