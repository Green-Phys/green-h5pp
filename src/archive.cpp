#include "green/h5pp/archive.h"

#include <filesystem>
#include <iostream>

green::h5pp::archive::archive(const std::string& filename, const std::string& access_type) :
    object(H5I_INVALID_HID, "/", FILE, access_type == "r"), _filename(filename) {
  open(filename, access_type);
}

void green::h5pp::archive::open(const std::string& filename, const std::string& access_type) {
  if (file_id() != H5I_INVALID_HID) {
    throw hdf5_file_access_error("File is already opened. Please close current file before opening another.");
  }
  if (access_type != "r" && access_type != "w" && access_type != "a") {
    throw hdf5_unknown_access_type_error("Unknown access type " + access_type + ". Should be 'r', 'w' or 'a'");
  }
  bool file_exists = std::filesystem::exists(filename);

  if (access_type == "r") {
    if (!file_exists) {
      throw hdf5_file_access_error("File " + filename + " does not exist.");
    }
    htri_t info = H5Fis_hdf5(filename.c_str());
    if (info < 0) {
      throw hdf5_file_access_error("Error accessing hdf5 file " + filename + ".");
    }
    if (info == 0) {
      throw not_hdf5_file_error(filename + " is not an HDF5 file.");
    }
  }
  hid_t file = H5I_INVALID_HID;
  if (access_type == "r")
    file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  else if (access_type == "a")
    file = file_exists ? H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT)
                       : H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  else if (access_type == "w")
    file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if (file == H5I_INVALID_HID) {
    throw hdf5_file_access_error("Can not open hdf5 file " + filename);
  }
  file_id()    = file;
  current_id() = file;
  readonly()   = access_type == "r";
}

green::h5pp::archive::~archive() {
  if (file_id() != H5I_INVALID_HID) H5Fclose(file_id());
}

bool green::h5pp::archive::close() {
  if (H5Fclose(file_id()) < 0) {
    throw hdf5_file_access_error("Can not close file " + _filename);
  }
  file_id()    = H5I_INVALID_HID;
  current_id() = H5I_INVALID_HID;
  return true;
}