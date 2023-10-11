#ifndef H5PP_ARCHIVE_H
#define H5PP_ARCHIVE_H

#include "object.h"

namespace green::h5pp {

  class archive : public object {
  public:
    archive() : object(H5I_INVALID_HID, H5I_INVALID_HID, "", FILE, false) {};
    archive(const std::string& filename, const std::string& access_type = "r");
    virtual ~archive();

    /**
     * Try to close hdf5 file
     *
     * @return true on success
     */
    bool close();

    /**
     * Open new file
     *
     * @param filename
     * @param access_type
     */
    void open(const std::string& filename, const std::string& access_type = "r");

  private:
    std::string _filename;
  };

}  // namespace green::h5pp

#endif  // H5PP_ARCHIVE_H
