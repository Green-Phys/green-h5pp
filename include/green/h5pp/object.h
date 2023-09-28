#ifndef H5PP_OBJECT_H
#define H5PP_OBJECT_H

#include <hdf5.h>
#include <hdf5_hl.h>

#include <iostream>
#include <numeric>
#include <string>

#include "except.h"
#include "type_traits.h"
#include "utils.h"

using namespace std::literals;

namespace green::h5pp {

  enum object_type { FILE, DATASET, GROUP, UNDEFINED, INVALID };

  namespace internal {
    inline void create_parents(hid_t root_parent, std::vector<std::string>& parents_list) {
      std::string current_root = "";
      for (auto parent : parents_list) {
        current_root += "/" + parent;
        htri_t check = H5Lexists(root_parent, current_root.c_str(), H5P_DEFAULT);
        if (check <= 0) {
          hid_t g_id = H5Gcreate2(root_parent, current_root.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
          H5Gclose(g_id);
        }
      }
    }

    template <typename T>
    std::pair<int, std::vector<size_t>> extract_dataset_shape(const T& rhs) {
      int                 rank = -1;
      std::vector<size_t> dims(0);
      if constexpr (is_scalar<T>) {
        rank = 0;
      } else if constexpr (is_1D_array<T>) {
        rank = 1;
        dims.push_back(rhs.size());
      } else if constexpr (is_ND_array<T>) {
        rank = rhs.shape().size();
        dims.resize(rank);
        std::copy(rhs.shape().begin(), rhs.shape().end(), dims.begin());
      } else {
        throw hdf5_unsupported_type_error("Type "s + typeid(T).name() +
                                          " is not supported in current implementation");
      }
      return std::make_pair(rank, dims);
    }

    template <typename T>
    struct hdf5_typename {};

    template <>
    struct hdf5_typename<int> {
      hid_t type = H5T_NATIVE_INT;
    };
    template <>
    struct hdf5_typename<long> {
      hid_t type = H5T_NATIVE_LONG;
    };
    template <>
    struct hdf5_typename<float> {
      hid_t type = H5T_NATIVE_FLOAT;
    };
    template <>
    struct hdf5_typename<double> {
      hid_t type = H5T_NATIVE_DOUBLE;
    };
    template <>
    struct hdf5_typename<std::string> {
      hid_t type = H5T_NATIVE_SCHAR;
    };
  }  // namespace internal

  inline bool dataset_exists(hid_t root_parent, const std::string& name) {
    htri_t check = H5Lexists(root_parent, name.c_str(), H5P_DEFAULT);
    if (check <= 0) {
      return false;
    }
    H5O_info2_t info;
    if (H5Oget_info_by_name(root_parent, name.c_str(), &info, H5O_INFO_BASIC | H5O_INFO_NUM_ATTRS,
                            H5P_DEFAULT) >= 0) {
      return info.type == H5O_TYPE_DATASET;
    }
    return false;
  }

  template <typename T>
  void write_scalar_dataset(hid_t d_id, const std::string& name, T rhs) {
    hid_t dataspace_id = H5Dget_space(d_id);
    H5Dwrite(d_id, internal::hdf5_typename<T>().type, H5S_ALL, dataspace_id, H5P_DEFAULT, &rhs);
    H5Sclose(dataspace_id);
  }

  template <typename T>
  hid_t get_type_id(const T& rhs) {
    return internal::hdf5_typename<T>().type;
  }

  template <typename T>
  hid_t get_type_id(const std::complex<T>& rhs) {
    hid_t tid = H5Tcreate(H5T_COMPOUND, sizeof(T) * 2);
    H5Tinsert(tid, "r", 0, internal::hdf5_typename<T>().type);
    H5Tinsert(tid, "i", sizeof(T), internal::hdf5_typename<T>().type);
    return tid;
  }

  template <typename T>
  void write_scalar_dataset(hid_t d_id, const std::string& name, std::complex<T> rhs) {
    hid_t type_id      = get_type_id(rhs);
    hid_t dataspace_id = H5Dget_space(d_id);
    H5Dwrite(d_id, type_id, H5S_ALL, dataspace_id, H5P_DEFAULT, &rhs);
    H5Sclose(dataspace_id);
  }

  hid_t create_group(hid_t root_parent, const std::string& name);

  template <typename T>
  hid_t create_scalar_dataset(hid_t root_parent, const std::string& name, T rhs) {
    std::vector<std::string> branch = utils::split(name, "/");
    std::vector<std::string> parents_list(branch.begin(), branch.end() - 1);
    internal::create_parents(root_parent, parents_list);
    hid_t dataspace_id = H5Screate(H5S_SCALAR);
    hid_t type_id      = get_type_id(rhs);
    hid_t d_id =
        H5Dcreate2(root_parent, name.c_str(), type_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (d_id == H5I_INVALID_HID) {
      throw hdf5_create_dataset_error("Can not create dataset " + name);
    }
    H5Sclose(dataspace_id);
    dataspace_id = H5Dget_space(d_id);
    H5Dwrite(d_id, type_id, H5S_ALL, dataspace_id, H5P_DEFAULT, &rhs);
    H5Sclose(dataspace_id);
    return d_id;
  }

  template <typename T>
  hid_t create_vector_dataset(hid_t root_parent, const std::string& name, T&& rhs) {
    std::vector<std::string> branch = utils::split(name, "/");
    std::vector<std::string> parents_list(branch.begin(), branch.end() - 1);
    internal::create_parents(root_parent, parents_list);
    auto [rank, int_dims] = internal::extract_dataset_shape(rhs);
    std::vector<hsize_t> dims(int_dims.begin(), int_dims.end());
    hid_t                dataspace_id = H5Screate_simple(rank, dims.data(), NULL);
    hid_t                type_id      = get_type_id(*rhs.data());
    hid_t                d_id =
        H5Dcreate2(root_parent, name.c_str(), type_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (d_id == H5I_INVALID_HID) {
      throw hdf5_create_dataset_error("Can not create dataset " + name);
    }
    H5Sclose(dataspace_id);
    dataspace_id = H5Dget_space(d_id);
    H5Dwrite(d_id, type_id, H5S_ALL, dataspace_id, H5P_DEFAULT, rhs.data());
    H5Sclose(dataspace_id);
    return d_id;
  }

  class object {
  public:
    object(const object&) = delete;
    object(object&& rhs) :
        _file_id(rhs._file_id), _current_id(rhs._current_id), _path(rhs._path), _type(rhs._type),
        _readonly(rhs._readonly) {
      rhs._file_id    = H5I_INVALID_HID;
      rhs._current_id = H5I_INVALID_HID;
    }

    object& operator=(const object&) = delete;
    object& operator=(object&& rhs) {
      if (&rhs == this) return *this;
      if (_current_id != H5I_INVALID_HID) {
        if (H5Oclose(_current_id) < 0)
          throw hdf5_object_close_error("Can not close "s + (_type == DATASET ? "dataset" : "group") + " " +
                                        _path);
      }
      // Transfer ownership of H5 object
      _current_id     = rhs._current_id;
      _file_id        = rhs._file_id;
      _path           = rhs._path;
      _type           = rhs._type;
      rhs._current_id = H5I_INVALID_HID;
      rhs._file_id    = H5I_INVALID_HID;
      return *this;
    }

    virtual ~object() noexcept(false) {
      if (_current_id == H5I_INVALID_HID) {
        return;
      }
      if (_type == INVALID || _type == FILE) {
        return;
      }
      if (H5Oclose(_current_id) < 0)
        throw hdf5_object_close_error("Can not close "s + (_type == DATASET ? "dataset" : "group") + " " +
                                      _path);
    }

    object(hid_t file_id, const std::string& opath, object_type otype, bool is_readonly) :
        _file_id(file_id), _path(opath), _type(otype), _readonly(is_readonly) {}

    object(hid_t file_id, hid_t current_id, const std::string& opath, object_type otype, bool is_readonly) :
        _file_id(file_id), _current_id(current_id), _path(opath), _type(otype), _readonly(is_readonly) {}

    object(hid_t file_id, hid_t parent_id, const std::string& path, const std::string& root_path,
           bool is_readonly) :
        _file_id(file_id),
        _current_id(H5I_INVALID_HID), _path(root_path), _type(INVALID), _readonly(is_readonly) {
      H5O_info_t oinfo;
      H5Oget_info_by_name(parent_id, path.c_str(), &oinfo, H5O_INFO_BASIC, H5P_DEFAULT);
      switch (oinfo.type) {
        case H5O_TYPE_GROUP:
          _type       = GROUP;
          _current_id = H5Gopen2(parent_id, path.c_str(), H5P_DEFAULT);
          break;
        case H5O_TYPE_DATASET:
          _type       = DATASET;
          _current_id = H5Dopen2(parent_id, path.c_str(), H5P_DEFAULT);
          break;
        default:
          _type = INVALID;
      }
      _path = root_path + "/" + path;
    }

    object operator[](const std::string& name) {
      if (_type != GROUP && _type != FILE && !(!_readonly && _type == UNDEFINED)) {
        throw hdf5_notsupported_error("Can not subscript.");
      }
      if (!_readonly && _type == UNDEFINED) {
        _current_id = create_group(_file_id, _path);
        _type       = GROUP;
      }
      htri_t info = H5LTpath_valid(_current_id, name.c_str(), true);
      if (info == 0) {
        if (_readonly) {
          throw hdf5_wrong_path_error("No valid HDF5 object for path " + _path + "/" + name);
        }
        return object(_file_id, H5I_INVALID_HID, _path + "/" + name, UNDEFINED, _readonly);
      }
      return object(_file_id, _current_id, name, _path, _readonly);
    }

    object operator[](const std::string& name) const {
      if (_type != GROUP || _type != FILE) {
        throw hdf5_notsupported_error("Can not subscript dataset.");
      }
      htri_t info = H5LTpath_valid(_current_id, name.c_str(), true);
      if (info == 0) {
        throw hdf5_wrong_path_error("No valid HDF5 object for path " + _path + "/" + name);
      }
      return object(_file_id, _current_id, name, _path, _readonly);
    }

    template <typename T>
    object& operator>>(T& rhs) {
      if (_type != DATASET) {
        throw hdf5_not_a_dataset_error(_path + " is not a dataset");
      }
      if constexpr (is_scalar<T>) {
        read_scalar(rhs);
      } else if constexpr (is_1D_array<T> || is_ND_array<T>) {
        read_vector(rhs);
      } else {
        throw hdf5_unsupported_type_error("Type "s + typeid(T).name() +
                                          " is not supported in current implementation"s);
      }
      return *this;
    }

    template <typename T>
    object& operator<<(T&& rhs) {
      if (_type != DATASET && _type != UNDEFINED) {
        throw std::runtime_error(_path + " is not dataset");
      }
      if (_type == UNDEFINED) {
        if constexpr (is_scalar<T>) {
          _current_id = create_scalar_dataset(_file_id, _path, rhs);
        } else if constexpr (is_1D_array<T> || is_ND_array<T>) {
          _current_id = create_vector_dataset(_file_id, _path, rhs);
        } else {
          throw hdf5_unsupported_type_error("Type "s + typeid(T).name() +
                                            " is not supported in current implementation"s);
        }
      } else {
        if constexpr (is_scalar<T>) {
          write_scalar_dataset(_current_id, _path, rhs);
        }
        // TODO: update existing dataset
      }
      _type = DATASET;
      return *this;
    }

    const std::string& path() const { return _path; }

    object_type        type() const { return _type; }

  protected:
    hid_t& file_id() { return _file_id; }
    hid_t  file_id() const { return _file_id; }
    hid_t& current_id() { return _current_id; }
    hid_t  current_id() const { return _current_id; }
    bool&  readonly() { return _readonly; }
    bool   readonly() const { return _readonly; }

  private:
    hid_t       _file_id;
    hid_t       _current_id;
    std::string _path;
    object_type _type;
    bool        _readonly;

    template <typename T>
    std::enable_if_t<is_scalar<T>, void> read_scalar(T& rhs) {
      hid_t   space_id;
      hsize_t ndims;
      space_id = H5Dget_space(_current_id);

      ndims    = H5Sget_simple_extent_ndims(space_id);
      std::vector<hsize_t> dims(ndims);
      H5Sget_simple_extent_dims(space_id, dims.data(), NULL);
      if (ndims != 0 && std::accumulate(dims.begin(), dims.end(), 1ul, std::multiplies<size_t>()) != 1) {
        throw hdf5_not_a_scalar_error("Dataset " + _path + " contains non scalar data.");
      }
      if (H5Tcompiler_conv(H5Dget_type(_current_id), get_type_id(rhs)) < 0) {
        throw hdf5_data_conversion_error("Can not convert data to specified type.");
      }
      read_scalar_internal(rhs);
    }
    template <typename T>
    void read_scalar_internal(T& rhs) const {
      if (H5Dread(_current_id, get_type_id(rhs), H5S_ALL, H5S_ALL, H5P_DEFAULT, &rhs) < 0)
        throw hdf5_read_error("Can not read dataset " + _path);
    }
    template <typename T>
    std::enable_if_t<!is_scalar<T>, void> read_vector(T& rhs) {
      hid_t   space_id;
      hsize_t ndims;
      space_id = H5Dget_space(_current_id);

      ndims    = H5Sget_simple_extent_ndims(space_id);
      std::vector<hsize_t> int_dims(ndims);
      H5Sget_simple_extent_dims(space_id, int_dims.data(), NULL);
      std::vector<size_t> dims(int_dims.begin(), int_dims.end());
      auto [src_rank, src_dims] = internal::extract_dataset_shape(rhs);
      if constexpr (is_1D_array<T>) {
        if constexpr (is_resizable<T>) {
          rhs.resize(dims[0]);
        } else {
          if (ndims != src_rank || dims != src_dims) {
            throw hdf5_read_error(
                "Target container's shape and dataset's shape are different and container cannot be "
                "resized.");
          }
        }
      } else if constexpr (is_ND_array<T>) {
        if constexpr (is_reshapable<T>) {
          rhs.reshape(dims);
        } else {
          if (ndims != src_rank || dims != src_dims) {
            throw hdf5_read_error(
                "Target container's shape and dataset's shape are different and container cannot be "
                "resized.");
          }
        }
      }
      if (H5Dread(_current_id, get_type_id(*rhs.data()), H5S_ALL, H5S_ALL, H5P_DEFAULT, rhs.data()) < 0)
        throw hdf5_read_error("Can not read dataset " + _path);
    }
  };
}  // namespace green::h5pp
#endif  // H5PP_OBJECT_H
