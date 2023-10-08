
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "green/h5pp/archive.h"

TEST_CASE("Archive") {
  SECTION("Open") {
    std::string root = TEST_PATH;
    REQUIRE_NOTHROW(green::h5pp::archive(root + "/test.h5"));
  }
  SECTION("Open Default Constructed") {
    std::string root = TEST_PATH;
    green::h5pp::archive ar;
    REQUIRE_NOTHROW(ar.open(root + "/test.h5"));
    REQUIRE_THROWS_AS(ar.open(root + "/test.h5"), green::h5pp::hdf5_file_access_error);
    ar.close();
    REQUIRE_NOTHROW(ar.open(root + "/test.h5"));
  }

  SECTION("Open for Write") {
    std::string          root           = TEST_PATH;
    std::string          file_to_create = root + "/test_create.h5";
    bool                 before         = std::filesystem::exists(file_to_create);
    green::h5pp::archive ar(file_to_create, "w");
    bool                 after = std::filesystem::exists(file_to_create);
    REQUIRE_FALSE(before == after);
    std::filesystem::remove(std::filesystem::path(file_to_create));
  }
  SECTION("Open Text File") {
    std::string root = TEST_PATH;
    // REQUIRE_THROWS_AS({green::h5pp::archive ar(root + "/test.txt")}, green::h5pp::not_hdf5_file_error);
  }
  SECTION("Open Wrong Path") {
    std::string root = TEST_PATH;
    // REQUIRE_THROWS_AS({green::h5pp::archive ar(root + "/test")}, green::h5pp::hdf5_file_access_error);
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
  }

  SECTION("Create Tree") {
    std::string          filename = TEST_PATH + "/test_write.h5"s;
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
  }
}
