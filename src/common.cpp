/*
 * Copyright (c) 2023 University of Michigan
 *
 */

#include "green/h5pp/common.h"

hid_t green::h5pp::create_group(hid_t root_parent, const std::string& name) {
  std::vector<std::string> parents_list = utils::split(name, "/");
  std::string              current_root;
  hid_t                    g_id = H5I_INVALID_HID;
  for (const auto& parent : parents_list) {
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

void green::h5pp::move_group(hid_t src_loc_id, const std::string& src_name, hid_t dst_loc_id, const std::string& dst_name) {
  herr_t herr = H5Gmove2(src_loc_id, src_name.c_str(), dst_loc_id, dst_name.c_str());
  if(herr < 0) {
    throw hdf5_move_group_error("Can not move group " + src_name + " to " + dst_name);
  }
}
