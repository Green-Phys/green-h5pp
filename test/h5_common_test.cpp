/*
 * Copyright (c) 2023 University of Michigan
 *
 */

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "green/h5pp/archive.h"
#include "test_common.h"

TEST_CASE("Common") {
  SECTION("Create Group") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/"s + random_name();
    green::h5pp::archive ar(file_to_create, "w");
    REQUIRE_FALSE(green::h5pp::group_exists(ar.file_id(), "TEST_GROUP"));
    green::h5pp::create_group(ar.file_id(), "TEST_GROUP");
    REQUIRE(green::h5pp::group_exists(ar.file_id(), "TEST_GROUP"));
  }

  SECTION("Create Dataset") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/"s + random_name();
    green::h5pp::archive ar(file_to_create, "w");
    REQUIRE_FALSE(green::h5pp::dataset_exists(ar.file_id(), "CHECK/CHECK/TEST_DATASET"));
    double x = 0;
    green::h5pp::create_dataset(ar.file_id(), "CHECK/CHECK/TEST_DATASET", x);
    std::filesystem::remove(file_to_create);
  }

}