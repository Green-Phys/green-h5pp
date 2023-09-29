//
// Created by iskakoff on 9/12/23.
//

#include "green/h5pp/common.h"

hid_t green::h5pp::create_group(hid_t root_parent, const std::string& name) {
  std::vector<std::string> parents_list = utils::split(name, "/");
  std::string              current_root = "";
  hid_t                    g_id         = H5I_INVALID_HID;
  for (auto parent : parents_list) {
    current_root += "/" + parent;
    htri_t check = H5Lexists(root_parent, current_root.c_str(), H5P_DEFAULT);
    if (check <= 0) {
      g_id = H5Gcreate2(root_parent, current_root.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      if (g_id == H5I_INVALID_HID) {
        throw hdf5_create_group_error("Can not create group " + name);
      }
      H5Gclose(g_id);
    }
  }
  g_id = H5Gopen2(root_parent, current_root.c_str(), H5P_DEFAULT);
  if (g_id == H5I_INVALID_HID) {
    throw hdf5_create_group_error("Can not create group " + name);
  }
  return g_id;
}
