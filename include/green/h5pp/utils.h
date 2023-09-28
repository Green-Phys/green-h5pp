//
// Created by iskakoff on 9/20/23.
//

#ifndef H5PP_UTILS_H
#define H5PP_UTILS_H

#include <string>
#include <vector>

namespace green::h5pp::utils {

  inline std::string ltrim(const std::string& s) {
    std::string res(s);
    res.erase(res.begin(),
              std::find_if(res.begin(), res.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return res;
  }
  inline std::string rtrim(const std::string& s) {
    std::string res(s);
    res.erase(
        std::find_if(res.rbegin(), res.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
        s.end());
    return res;
  }
  inline std::string              trim(const std::string& s) { return rtrim(ltrim(s)); }
  inline std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> res;
    size_t                   pos     = 0;
    size_t                   new_pos = 0;
    while (true) {
      if ((new_pos = s.find(delimiter, pos)) == std::string::npos) {
        auto str_to_add = s.substr(pos, s.length());
        res.push_back(str_to_add);
        return res;
      }
      auto str_to_add = s.substr(pos, new_pos - pos);
      if (str_to_add.length() != 0) {
        res.push_back(str_to_add);
      }
      pos = new_pos + 1;
    }
    return res;
  }

}  // namespace green::h5pp::utils

#endif  // H5PP_UTILS_H
