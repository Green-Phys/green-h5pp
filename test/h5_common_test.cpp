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
    std::filesystem::remove(file_to_create);
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

  SECTION("Create When Already Exists") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/"s + random_name();
    green::h5pp::archive ar(file_to_create, "w");
    double               x = 0;
    green::h5pp::create_dataset(ar.file_id(), "TEST_DATASET", x);
    green::h5pp::create_group(ar.file_id(), "TEST_GROUP");
    REQUIRE_THROWS_AS(green::h5pp::create_dataset(ar.file_id(), "TEST_GROUP", x), green::h5pp::hdf5_create_dataset_error);
    REQUIRE_THROWS_AS(green::h5pp::create_group(ar.file_id(), "TEST_DATASET"), green::h5pp::hdf5_create_group_error);
    std::filesystem::remove(file_to_create);
  }

  SECTION("Move Group") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/"s + random_name();
    green::h5pp::archive ar(file_to_create, "w");
    green::h5pp::create_group(ar.file_id(), "TEST_GROUP");
    REQUIRE_FALSE(green::h5pp::group_exists(ar.file_id(), "TEST_GROUP2"));
    green::h5pp::move_group(ar.file_id(), "TEST_GROUP", ar.file_id(), "TEST_GROUP2");
    REQUIRE(green::h5pp::group_exists(ar.file_id(), "TEST_GROUP2"));
    std::filesystem::remove(file_to_create);
  }

  SECTION("Create attribute") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/"s + random_name();
    green::h5pp::archive ar(file_to_create, "w");
    REQUIRE_FALSE(ar.has_attribute("test_attr"));
    ar.close();
    ar.open(file_to_create, "r");
    REQUIRE_THROWS_AS(ar.set_attribute("test_attr", "test"s), green::h5pp::hdf5_write_error);
    ar.close();
    ar.open(file_to_create, "a");
    std::string test_attr_value = "AAA";
    ar.set_attribute("test_attr", test_attr_value);

    REQUIRE(ar.has_attribute("test_attr"));
    REQUIRE_THROWS_AS(ar.get_attribute<std::string>("test_attr1"), green::h5pp::hdf5_read_error);
    REQUIRE(ar.get_attribute<std::string>("test_attr") == test_attr_value);
    std::string test_attr_value2 = "BBB";
    ar.set_attribute("test_attr", test_attr_value2);
    REQUIRE(ar.get_attribute<std::string>("test_attr") == test_attr_value2);
    REQUIRE_THROWS_AS(ar.set_attribute("test_attr", 111), green::h5pp::hdf5_data_conversion_error);
    REQUIRE_THROWS_AS(ar.get_attribute<int>("test_attr"), green::h5pp::hdf5_data_conversion_error);
    REQUIRE_NOTHROW(ar.set_attribute("test_attr2", 12));
    REQUIRE_NOTHROW(ar.get_attribute<double>("test_attr2"));
    std::filesystem::remove(file_to_create);
  }
}
