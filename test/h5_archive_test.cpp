/*
 * Copyright (c) 2023 University of Michigan
 *
 */

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "green/h5pp/archive.h"
#include "test_common.h"

TEST_CASE("Archive") {
  SECTION("Open") {
    std::string root = TEST_PATH;
    REQUIRE_NOTHROW(green::h5pp::archive(root + "/test.h5"));
  }
  SECTION("Open Default Constructed") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar;
    REQUIRE_NOTHROW(ar.open(root + "/test.h5"));
    REQUIRE_THROWS_AS(ar.open(root + "/test.h5"), green::h5pp::hdf5_file_access_error);
    ar.close();
    REQUIRE_NOTHROW(ar.open(root + "/test.h5"));
  }

  SECTION("Open for Write") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/"s + random_name();
    bool                 before         = std::filesystem::exists(file_to_create);
    green::h5pp::archive ar(file_to_create, "w");
    bool                 after = std::filesystem::exists(file_to_create);
    REQUIRE_FALSE(before == after);
    std::filesystem::remove(std::filesystem::path(file_to_create));
  }
  SECTION("Open for Unknown") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/"s + random_name();
    green::h5pp::archive ar;
    REQUIRE_THROWS_AS(ar.open(file_to_create, "T"), green::h5pp::hdf5_unknown_access_type_error);
  }
  SECTION("Open Text File") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar;
    REQUIRE_THROWS_AS(ar.open(root + "/test.txt"), green::h5pp::not_hdf5_file_error);
  }
  SECTION("Open Wrong Path") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar;
    REQUIRE_THROWS_AS(ar.open(root + "/test"), green::h5pp::hdf5_file_access_error);
  }
  SECTION("Get Group") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    auto                 group = ar["GROUP"];
    REQUIRE(group.type() == green::h5pp::GROUP);
    auto inner_group = ar["GROUP/INNER_GROUP"];
    REQUIRE(inner_group.type() == green::h5pp::GROUP);
    inner_group = group["INNER_GROUP"];
    REQUIRE(inner_group.type() == green::h5pp::GROUP);
  }
  SECTION("Get Group for const Archive") {
    std::string                root = TEST_PATH;
    const green::h5pp::archive ar(root + "/test.h5");
    auto                       group = ar["GROUP"];
    REQUIRE(group.type() == green::h5pp::GROUP);
  }
  SECTION("Get Wrong Group") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    REQUIRE_THROWS_AS(ar["GRP"], green::h5pp::hdf5_wrong_path_error);
    {
      const auto& ar2 = ar;
      REQUIRE_THROWS_AS(ar2["GRP"], green::h5pp::hdf5_wrong_path_error);
    }
  }
  SECTION("Check Group Existence") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5", "a");
    REQUIRE(ar.has_group("GROUP"));
    REQUIRE_FALSE(ar.has_group("GRP"));
    auto gr = ar["GROUP"];
    REQUIRE(gr.has_group("INNER_GROUP"));
    REQUIRE_FALSE(ar["GRP"].has_group("INNER_GROUP"));
  }
  SECTION("Check Dataset Existence") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5", "a");
    REQUIRE(ar.is_data("GROUP/SCALAR_DATASET"));
    REQUIRE_FALSE(ar.is_data("GRP"));
    REQUIRE_FALSE(ar.is_data("GRP/DATA"));
    auto gr = ar["GROUP"];
    REQUIRE(gr.is_data("SCALAR_DATASET"));
    REQUIRE_FALSE(ar["GRP"].is_data("DATASET"));
  }
  SECTION("Get Dataset") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    auto                 dataset = ar["GROUP/VECTOR_DATASET"];
    REQUIRE(dataset.type() == green::h5pp::DATASET);
    dataset = ar["GROUP"]["INNER_GROUP/DATASET"];
    REQUIRE(dataset.type() == green::h5pp::DATASET);
    REQUIRE_THROWS_AS(dataset["TEST"], green::h5pp::hdf5_notsupported_error);
  }

  SECTION("Create Tree") {
    std::string          filename = TEST_PATH + "/"s + random_name();
    green::h5pp::archive ar(filename, "w");
    auto                 group = ar["GROUP/TEST"];
    REQUIRE(group.type() == green::h5pp::UNDEFINED);
    auto inner_group = group["INNER_GROUP"];
    REQUIRE(inner_group.type() == green::h5pp::UNDEFINED);
    group = ar["GROUP"];
    REQUIRE(group.type() == green::h5pp::GROUP);
    std::filesystem::remove(std::filesystem::path(filename));
  }
  SECTION("Close File") {
    std::string          root = TEST_PATH;
    green::h5pp::archive ar(root + "/test.h5");
    REQUIRE(ar.is_valid());
    REQUIRE(ar.close());
    REQUIRE_FALSE(ar.is_valid());
    REQUIRE_THROWS_AS(ar.close(), green::h5pp::hdf5_file_access_error);
    ar.file_id()    = 55555;
    ar.current_id() = 55555;
    green::h5pp::archive ar2(root + "/test.h5");
    REQUIRE_THROWS_AS(ar = ar2, green::h5pp::hdf5_object_close_error);
    REQUIRE_THROWS_AS(ar.close(), green::h5pp::hdf5_file_access_error);
    ar.file_id()    = H5I_INVALID_HID;
    ar.current_id() = H5I_INVALID_HID;
  }
}
