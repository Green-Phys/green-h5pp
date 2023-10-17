/*
 * Copyright (c) 2023 University of Michigan
 *
 */

#ifndef H5PP_TEST_COMMON_H
#define H5PP_TEST_COMMON_H

#include <random>
#include <string>

inline std::string random_name() {
  std::string        str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  std::random_device rd;
  std::mt19937       generator(rd());
  std::shuffle(str.begin(), str.end(), generator);
  return str.substr(0, 32) + ".h5";  // assumes 32 < number of characters in str
}
#endif  // H5PP_TEST_COMMON_H
