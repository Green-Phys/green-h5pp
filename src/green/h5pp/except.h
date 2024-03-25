/*
 * Copyright (c) 2023 University of Michigan
 *
 */

#ifndef H5PP_ERRORS_H
#define H5PP_ERRORS_H

#include <stdexcept>

namespace green::h5pp {
  class not_hdf5_file_error : public std::runtime_error {
  public:
    not_hdf5_file_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_file_access_error : public std::runtime_error {
  public:
    hdf5_file_access_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_wrong_path_error : public std::runtime_error {
  public:
    hdf5_wrong_path_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_not_a_scalar_error : public std::runtime_error {
  public:
    hdf5_not_a_scalar_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_not_a_dataset_error : public std::runtime_error {
  public:
    hdf5_not_a_dataset_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_read_error : public std::runtime_error {
  public:
    hdf5_read_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_write_error : public std::runtime_error {
  public:
    hdf5_write_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_unsupported_type_error : public std::runtime_error {
  public:
    hdf5_unsupported_type_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_unknown_access_type_error : public std::runtime_error {
  public:
    hdf5_unknown_access_type_error(const std::string& string) : runtime_error(string) {}
  };

  class hdf5_notsupported_error : public std::runtime_error {
  public:
    hdf5_notsupported_error(const std::string& string) : runtime_error(string) {}
  };
  class hdf5_create_group_error : public std::runtime_error {
  public:
    hdf5_create_group_error(const std::string& string) : runtime_error(string) {}
  };
  class hdf5_move_group_error : public std::runtime_error {
  public:
    hdf5_move_group_error(const std::string& string) : runtime_error(string) {}
  };
  class hdf5_create_dataset_error : public std::runtime_error {
  public:
    hdf5_create_dataset_error(const std::string& string) : runtime_error(string) {}
  };
  class hdf5_data_conversion_error : public std::runtime_error {
  public:
    hdf5_data_conversion_error(const std::string& string) : runtime_error(string) {}
  };
  class hdf5_object_close_error : public std::runtime_error {
  public:
    hdf5_object_close_error(const std::string& string) : runtime_error(string) {}
  };

}  // namespace green::h5pp

#endif  // H5PP_ERRORS_H
