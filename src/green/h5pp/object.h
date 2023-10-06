#ifndef H5PP_OBJECT_H
#define H5PP_OBJECT_H

#include <iostream>
#include <string>

#include "common.h"

namespace green::h5pp {

  enum object_type { FILE, DATASET, GROUP, UNDEFINED, INVALID };

  class object {
  public:
    /**
     * Default constructor for H5 object set all ids to H5I_INVALID_HID
     */
    object() : _file_id(H5I_INVALID_HID), _current_id(H5I_INVALID_HID), _path(""), _type(INVALID), _readonly(false) {}
    /**
     * Copy-constructor. We open new H5 obejct at the specific path to avoid multiple release of the same resource
     * @param rhs - object to make copy from
     */
    object(const object& rhs) :
        _file_id(rhs._file_id), _current_id(H5I_INVALID_HID), _path(rhs._path), _type(rhs._type), _readonly(rhs._readonly) {
      _current_id = H5Oopen(_file_id, _path.c_str(), H5P_DEFAULT);
    }
    /**
     * Move constructor. We make sure that we invalidate source ids
     * @param rhs - object to be moved
     */
    object(object&& rhs) :
        _file_id(rhs._file_id), _current_id(rhs._current_id), _path(rhs._path), _type(rhs._type), _readonly(rhs._readonly) {
      rhs._file_id    = H5I_INVALID_HID;
      rhs._current_id = H5I_INVALID_HID;
    }
    /**
     * Copy-assignment. Similar to constructor we open new object with specific path. We make sure to release current
     * resources if they have valid H5 id
     *
     * @param rhs - object to make copy from
     * @return reference to current object
     */
    object& operator=(const object& rhs) {
      if (&rhs == this) return *this;
      if (_current_id != H5I_INVALID_HID) {
        if (H5Oclose(_current_id) < 0)
          throw hdf5_object_close_error("Can not close "s + (_type == DATASET ? "dataset" : "group") + " " + _path);
      }
      _file_id    = rhs._file_id;
      _current_id = H5Oopen(_file_id, rhs._path.c_str(), H5P_DEFAULT);
      _path       = rhs._path;
      _type       = rhs._type;
      _readonly   = rhs._readonly;
      return *this;
    }
    /**
     * Move assignment. Source object is invalidated. We make sure to release current resources if they have valid H5 id.
     *
     * @param rhs - object to be moved
     * @return reference to current object
     */
    object& operator=(object&& rhs) {
      if (&rhs == this) return *this;
      if (_current_id != H5I_INVALID_HID) {
        if (H5Oclose(_current_id) < 0)
          throw hdf5_object_close_error("Can not close "s + (_type == DATASET ? "dataset" : "group") + " " + _path);
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

    /**
     * Check if the object id is valid and close it. This only close dataset and group, file is closed in it's own destructor.
     */
    virtual ~object() noexcept(false) {
      if (_current_id == H5I_INVALID_HID) {
        return;
      }
      if (_type == INVALID || _type == FILE) {
        return;
      }
      if (H5Oclose(_current_id) < 0)
        throw hdf5_object_close_error("Can not close "s + (_type == DATASET ? "dataset" : "group") + " " + _path);
    }

    /**
     * Construct object with specific file_id, path and type
     *
     * @param file_id - id of the file
     * @param opath - path to the object
     * @param otype - type of the object
     * @param is_readonly - set to be read-only
     */
    object(hid_t file_id, const std::string& opath, object_type otype, bool is_readonly) :
        _file_id(file_id), _path(opath), _type(otype), _readonly(is_readonly) {}

    /**
     * Construct object for specific H5 id and with specific file_id, path and type
     *
     * @param file_id - id of the file
     * @param current_id - id of a current group or dataset
     * @param opath - path to the object
     * @param otype - type of the object
     * @param is_readonly - set to be read-only
     */
    object(hid_t file_id, hid_t current_id, const std::string& opath, object_type otype, bool is_readonly) :
        _file_id(file_id), _current_id(current_id), _path(opath), _type(otype), _readonly(is_readonly) {}

    /**
     * Create object for specific parent object.
     *
     * @param file_id - id of the file
     * @param parent_id - id of the parent object
     * @param path - relative path of the object, should be a path to a valid group or dataset
     * @param root_path - path to the parent object
     * @param is_readonly - set to be read-only
     */
    object(hid_t file_id, hid_t parent_id, const std::string& path, const std::string& root_path, bool is_readonly) :
        _file_id(file_id), _current_id(H5I_INVALID_HID), _path(root_path + "/" + path), _type(INVALID), _readonly(is_readonly) {
      hdf5_info_t oinfo;
      H5Oget_info_by_name2(parent_id, path.c_str(), &oinfo, H5O_INFO_BASIC, H5P_DEFAULT);
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
    }

    /**
     * Subscript operator. This operator can only be called on `FILE', `GROUP' or `UNDEFINED' object type.
     * New group will be created and object type will be set to `GROUP` if object type is undefined and file is not
     * open for read-only. New H5 object with relative path `name' will be returned. If `name' is existing dataset or group path,
     * the corresponding H5 object will be open. Object of `UNDEFINED' type will be returned otherwise.
     *
     * @param name - relative path to a group or dataset
     * @return group, dataset or undefined object for specific relative path
     */
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

    /**
     * Constant version of subscript operator. Check if the current object is valid `FILE' or `GROUP' and that `name' corresponds
     * to a valid realtive path in current file. Valid H5 object will be returned.
     *
     * @param name - relative path to a group or dataset
     * @return valid group or dataset object for specific relative path
     */
    object operator[](const std::string& name) const {
      if (_type != GROUP && _type != FILE) {
        throw hdf5_notsupported_error("Only File or Group can subscripted.");
      }
      htri_t info = H5LTpath_valid(_current_id, name.c_str(), true);
      if (info == 0) {
        throw hdf5_wrong_path_error("No valid HDF5 object for path " + _path + "/" + name);
      }
      return object(_file_id, _current_id, name, _path, _readonly);
    }

    /**
     * Read data from current dataset into `rhs' variable. Check that object is dataset.
     *
     * @tparam T - type of variable
     * @param rhs - variable to read rata into
     * @return current object to chain reading.
     */
    template <typename T>
    object& operator>>(T& rhs) {
      if (_type != DATASET) {
        throw hdf5_not_a_dataset_error(_path + " is not a dataset");
      }
      if constexpr (is_scalar<T> || is_1D_array<T> || is_ND_array<T>) {
        read_dataset(_current_id, _path, rhs);
      } else if constexpr (is_string<T>) {
        read_string_dataset(_current_id, _path, rhs);
      } else {
        throw hdf5_unsupported_type_error("Type "s + typeid(T).name() + " is not supported in current implementation"s);
      }
      return *this;
    }

    /**
     * Write `rhs' into current dataset. If object has `UNDEFINED' type new dataset will be created.
     * Data will be overwritten if dataset already exists and source and target shape matching.
     *
     * @tparam T - type of data to be written
     * @param rhs - data to be written
     * @return current object to chain writting
     */
    template <typename T>
    object& operator<<(T&& rhs) {
      if (_readonly) {
        throw hdf5_write_error("Can not write into readonly object");
      }
      if (_type != DATASET && _type != UNDEFINED) {
        throw std::runtime_error(_path + " is not dataset");
      }
      if (_type == UNDEFINED) {
        if constexpr (is_scalar<T> || is_1D_array<T> || is_ND_array<T> || is_string<T>) {
          _current_id = create_dataset(_file_id, _path, rhs);
        } else {
          throw hdf5_unsupported_type_error("Type "s + typeid(T).name() + " is not supported in current implementation"s);
        }
      } else {
        if constexpr (is_scalar<T> || is_1D_array<T> || is_ND_array<T>) {
          write_dataset(_current_id, _path, rhs);
        } else {
          throw hdf5_unsupported_type_error("Type "s + typeid(T).name() + " is not supported in current implementation"s);
        }
      }
      _type = DATASET;
      return *this;
    }

    /**
     * @return Absolute path to the object
     */
    const std::string& path() const { return _path; }

    /**
     * @return type of the object
     */
    object_type type() const { return _type; }

    /**
     * @return `true' if group `group_name' exists
     */
    bool has_group(const std::string& group_name) const {
      if (_current_id == H5I_INVALID_HID) return false;
      return group_exists(_current_id, group_name[0] != '/' ? _path + "/" + group_name : group_name);
    }

    /**
     * Check if object state is valid
     * @return true if object is valid
     */
    bool is_valid() const { return _file_id != H5I_INVALID_HID || _current_id != H5I_INVALID_HID; }

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
  };
}  // namespace green::h5pp
#endif  // H5PP_OBJECT_H
