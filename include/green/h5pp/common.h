#ifndef H5PP_COMMON_H
#define H5PP_COMMON_H

#include <hdf5.h>
#include <hdf5_hl.h>

#include <numeric>

#include "except.h"
#include "type_traits.h"
#include "utils.h"

using namespace std::literals;

#if H5O_info_t_vers == 2
#define hdf5_info_t H5O_info1_t
#else
#define hdf5_info_t H5O_info_t
#endif

namespace green::h5pp {
  namespace internal {
    /**
     * Create all necessary parent groups for current new H5 object
     *
     * @param root_parent - h5 id of the parent object the current object was created
     * @param parents_list - the full list of parent objects
     */
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

    /**
     * Setup intended dimension and shape for the target dataset
     *
     * @tparam T - datatype of the source data
     * @param rhs - source data, can be scalar, array, vector or multi-dimensional array
     * @return pair of dimension and shape of target dataset
     */
    template <typename T>
    std::pair<int, std::vector<size_t>> extract_dataset_shape(const T& rhs) {
      int                 rank = -1;
      std::vector<size_t> dims(0);
      if constexpr (is_scalar<T> || is_string<T>) {
        rank = 0;
      } else if constexpr (is_1D_array<T>) {
        rank = 1;
        dims.push_back(rhs.size());
      } else if constexpr (is_ND_array<T>) {
        rank = rhs.shape().size();
        dims.resize(rank);
        std::copy(rhs.shape().begin(), rhs.shape().end(), dims.begin());
      } else {
        throw hdf5_unsupported_type_error("Type "s + typeid(T).name() + " is not supported in current implementation");
      }
      return std::make_pair(rank, dims);
    }

    template <typename T>
    struct hdf5_typename {
      static hid_t type;
    };

    template <>
    inline hid_t hdf5_typename<int>::type = H5T_NATIVE_INT;
    template <>
    inline hid_t hdf5_typename<long>::type = H5T_NATIVE_LONG;
    template <>
    inline hid_t hdf5_typename<float>::type = H5T_NATIVE_FLOAT;
    template <>
    inline hid_t hdf5_typename<double>::type = H5T_NATIVE_DOUBLE;
    template <>
    inline hid_t hdf5_typename<std::string>::type = H5T_NATIVE_SCHAR;
  }  // namespace internal

  /**
   * Check if dataset with name `name' exists
   *
   * @param root_parent - hdf5 id of the root group or file
   * @param name - name of the dataset
   * @return `true' if dataset exists
   */
  inline bool dataset_exists(hid_t root_parent, const std::string& name) {
    htri_t check = H5Lexists(root_parent, name.c_str(), H5P_DEFAULT);
    if (check <= 0) {
      return false;
    }
    hdf5_info_t info;
    if (H5Oget_info_by_name2(root_parent, name.c_str(), &info, H5O_INFO_BASIC | H5O_INFO_NUM_ATTRS, H5P_DEFAULT) >= 0) {
      return info.type == H5O_TYPE_DATASET;
    }
    return false;
  }

  /**
   * Get H5 type id for the scalar data
   *
   * @tparam T type of the object
   * @param rhs - scalar data
   * @return corresponding H5 type id for the data
   */
  template <typename T>
  std::enable_if_t<is_scalar<T> && !is_complex_scalar<T>, hid_t> get_type_id(const T& rhs) {
    return internal::hdf5_typename<std::remove_const_t<T>>::type;
  }

  /**
   * Get H5 type id for the std::complex scalar data
   *
   * @tparam T std::complex value type
   * @param rhs - source data
   * @return H5 compound type for std::complex
   */
  template <typename T>
  hid_t get_type_id(const std::complex<T>& rhs) {
    hid_t tid = H5Tcreate(H5T_COMPOUND, sizeof(T) * 2);
    H5Tinsert(tid, "r", 0, internal::hdf5_typename<T>::type);
    H5Tinsert(tid, "i", sizeof(T), internal::hdf5_typename<T>::type);
    return tid;
  }

  inline hid_t get_type_id(const std::string& rhs) {
    hid_t tid = H5Tcopy(H5T_C_S1);
    H5Tset_size(tid, H5T_VARIABLE);
    H5Tset_cset(tid, H5T_CSET_UTF8);
    return tid;
  }

  /**
   * Get underlying H5 type for 1+ dimensional data
   * @tparam T - non-scalar datatype
   * @param rhs - data container
   * @return corresponding H5 data type id for container elements
   */
  template <typename T>
  std::enable_if_t<is_1D_array<T> | is_ND_array<T>, hid_t> get_type_id(const T& rhs) {
    return get_type_id(*rhs.data());
  }

  /**
   * Write `rhs' into dataset with id=d_id. For scalar `rhs' `hdf5_not_a_scalar_error' will be thrown if
   * target dataset is not scalar or has more than a single element. For 1+ dimensional `rhs' `hdf5_write_error' will be
   * thrown if source and target size/shape missmatched.
   *
   * @tparam T - type of the source data
   * @param d_id - dataset id
   * @param rhs - data to be written into dataset
   */
  template <typename T>
  void write_dataset(hid_t d_id, const std::string& path, T&& rhs) {
    hid_t                type_id      = get_type_id(rhs);
    hid_t                dataspace_id = H5Dget_space(d_id);
    size_t               dst_rank     = H5Sget_simple_extent_ndims(dataspace_id);
    std::vector<hsize_t> int_dims(dst_rank);
    H5Sget_simple_extent_dims(dataspace_id, int_dims.data(), NULL);
    std::vector<size_t> dst_dims(int_dims.begin(), int_dims.end());
    auto [src_rank, src_dims] = internal::extract_dataset_shape(rhs);
    if constexpr (is_scalar<T>) {
      if (dst_rank != 0 && std::accumulate(dst_dims.begin(), dst_dims.end(), 1ul, std::multiplies<size_t>()) != 1) {
        throw hdf5_not_a_scalar_error("Dataset " + path + " contains non scalar data.");
      }
    } else if constexpr (is_1D_array<T>) {
      if (std::accumulate(dst_dims.begin(), dst_dims.end(), 1ul, std::multiplies<size_t>()) !=
          std::accumulate(src_dims.begin(), src_dims.end(), 1ul, std::multiplies<size_t>())) {
        throw hdf5_write_error("Source container's shape and dataset " + path + "'s shape are different.");
      }
    } else if constexpr (is_ND_array<T>) {
      if (dst_rank != src_rank || dst_dims != src_dims) {
        throw hdf5_write_error("Source container's shape and dataset " + path + "'s shape are different.");
      }
    }
    H5Dwrite(d_id, type_id, H5S_ALL, dataspace_id, H5P_DEFAULT, &rhs);
    H5Sclose(dataspace_id);
  }

  /**
   * Create group with path `name' with all necessary parent groups. `name' can contain `/' to create
   * group tree. If `name' starts with `/' the absolute path is used, the path relative to `root_parent'
   * is used otherwise.
   *
   * @param root_parent - direct parent id
   * @param name - path for new group
   * @return id of newly created group
   */
  hid_t create_group(hid_t root_parent, const std::string& name);

  /**
   * Create dataset at `name' path for `root_parent' object and write `rhs' data into it.
   * Absolute path is used if `name' starts with `/', otherwise the path is relative to `root_parent'.
   * All parent groups willl be created if needed.
   *
   * @tparam T - type of the data to be written
   * @param root_parent - id of the parent group
   * @param name - path of the dataset to be written
   * @param rhs
   * @return
   */
  template <typename T>
  hid_t create_dataset(hid_t root_parent, const std::string& name, T&& rhs) {
    std::vector<std::string> branch = utils::split(name, "/");
    std::vector<std::string> parents_list(branch.begin(), branch.end() - 1);
    internal::create_parents(root_parent, parents_list);
    auto [rank, int_dims] = internal::extract_dataset_shape(rhs);
    std::vector<hsize_t> dims(int_dims.begin(), int_dims.end());
    hid_t                dataspace_id = is_scalar<T> ? H5Screate(H5S_SCALAR) : H5Screate_simple(rank, dims.data(), NULL);
    hid_t                type_id      = get_type_id(rhs);
    // else type_id = get_type_id(*rhs.data());
    hid_t d_id = H5Dcreate2(root_parent, name.c_str(), type_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (d_id == H5I_INVALID_HID) {
      throw hdf5_create_dataset_error("Can not create dataset " + name);
    }
    const void* data;
    if constexpr (is_scalar<T>)
      data = &rhs;
    else if constexpr (is_string<T>)
      data = rhs.c_str();
    else
      data = rhs.data();
    H5Sclose(dataspace_id);
    dataspace_id = H5Dget_space(d_id);
    if constexpr (is_string<T>)
      H5Dwrite(d_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data);
    else
      H5Dwrite(d_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    H5Sclose(dataspace_id);
    return d_id;
  }

  /**
   * Read `current_id' dataset. For 1+ dimensional objects container size/shape willbe adjusted if the container datatype
   * allows resize/reshape. For scalar object we check that data is either 0-dimensional or has only a single element.
   *
   * @tparam T - type of target data
   * @param current_id - id of dataset to be read
   * @param path - absolute path to dataset (needed for error message)
   * @param rhs - target data container
   */
  template <typename T>
  void read_dataset(hid_t current_id, const std::string& path, T& rhs) {
    hid_t   space_id;
    hsize_t src_rank;
    space_id = H5Dget_space(current_id);

    src_rank = H5Sget_simple_extent_ndims(space_id);
    std::vector<hsize_t> int_dims(src_rank);
    H5Sget_simple_extent_dims(space_id, int_dims.data(), NULL);
    std::vector<size_t> src_dims(int_dims.begin(), int_dims.end());
    auto [dst_rank, dst_dims] = internal::extract_dataset_shape(rhs);
    if constexpr (is_scalar<T>) {
      if (src_rank != 0 && std::accumulate(src_dims.begin(), src_dims.end(), 1ul, std::multiplies<size_t>()) != 1) {
        throw hdf5_not_a_scalar_error("Dataset " + path + " contains non scalar data.");
      }
    } else if constexpr (is_1D_array<T>) {
      if constexpr (is_resizable<T>) {
        rhs.resize(std::accumulate(src_dims.begin(), src_dims.end(), 1ul, std::multiplies<size_t>()));
      } else {
        if (std::accumulate(dst_dims.begin(), dst_dims.end(), 1ul, std::multiplies<size_t>()) !=
            std::accumulate(src_dims.begin(), src_dims.end(), 1ul, std::multiplies<size_t>())) {
          throw hdf5_read_error("Target container's shape and dataset's shape are different and container cannot be resized.");
        }
      }
    } else if constexpr (is_ND_array<T>) {
      if constexpr (is_reshapable<T>) {
        rhs.reshape(src_dims);
      } else {
        if (src_rank != dst_rank || src_dims != dst_dims) {
          throw hdf5_read_error("Target container's shape and dataset's shape are different and container cannot be resized.");
        }
      }
    }
    if (H5Tcompiler_conv(H5Dget_type(current_id), get_type_id(rhs)) < 0) {
      throw hdf5_data_conversion_error("Can not convert data to specified type.");
    }
    void* data;
    if constexpr (is_scalar<T>)
      data = &rhs;
    else
      data = rhs.data();
    if (H5Dread(current_id, get_type_id(rhs), H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0)
      throw hdf5_read_error("Can not read dataset " + path);
  }

  /**
   * Read string dataset. Variable string dataset have slightly different sintax that basic types.
   *
   * @tparam T - string type
   * @param current_id - dataset id
   * @param path - absolute path to dataset
   * @param rhs - string to read data into
   */
  template <typename T>
  void read_string_dataset(hid_t current_id, const std::string& path, T& rhs) {
    hid_t   space_id;
    hsize_t src_rank;
    space_id = H5Dget_space(current_id);

    src_rank = H5Sget_simple_extent_ndims(space_id);
    std::vector<hsize_t> int_dims(src_rank);
    H5Sget_simple_extent_dims(space_id, int_dims.data(), NULL);
    std::vector<size_t> src_dims(int_dims.begin(), int_dims.end());
    auto [dst_rank, dst_dims] = internal::extract_dataset_shape(rhs);
    if constexpr (is_scalar<T>) {
      if (src_rank != 0 && std::accumulate(src_dims.begin(), src_dims.end(), 1ul, std::multiplies<size_t>()) != 1) {
        throw hdf5_not_a_scalar_error("Dataset " + path + " contains non scalar data.");
      }
    }
    hid_t tid = H5Dget_type(current_id);
    assert(H5Tget_class(tid) == H5T_STRING);

    if (H5Tis_variable_str(tid)) {
      char*       rd_ptr[1];
      std::string s;
      if (H5Dread(current_id, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, rd_ptr) < 0)
        throw hdf5_read_error("Cannot read the string " + path);
      s.append(*rd_ptr);

      // Free the resources allocated in the variable length read
      if (H5Dvlen_reclaim(tid, space_id, H5P_DEFAULT, rd_ptr) < 0)
        throw hdf5_read_error("Cannot free resources for variable-length string type");
      rhs = s;
    } else {
      throw hdf5_read_error("Only variable length strings are supported.");
    }
  }
}  // namespace green::h5pp

#endif  // H5PP_COMMON_H
