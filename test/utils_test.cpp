/*
 * Copyright (c) 2023 University of Michigan
 *
 */

#include "green/h5pp/utils.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Utils") {
  SECTION("Test Trim") {
    std::string test = green::h5pp::utils::ltrim(" aaa  ");
    REQUIRE(test == "aaa  ");
    REQUIRE(green::h5pp::utils::rtrim(test) == "aaa");
    REQUIRE(green::h5pp::utils::trim("  aaa  ") == "aaa");
  }

  SECTION("Test Split") {
    std::vector<std::string> test = green::h5pp::utils::split("aaa/bbb//ccc/ddd", "/");
    REQUIRE(test.size() == 4);
    REQUIRE(test[0] == "aaa");
    REQUIRE(test[1] == "bbb");
    REQUIRE(test[2] == "ccc");
    REQUIRE(test[3] == "ddd");
    test = green::h5pp::utils::split("aaa", "/");
    REQUIRE(test.size() == 1);
    REQUIRE(test[0] == "aaa");
  }
}
