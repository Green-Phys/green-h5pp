#ifndef H5PP_ARCHIVE_H
#define H5PP_ARCHIVE_H

#include "object.h"

namespace green::h5pp {

  class archive : public object {
  public:
    archive(const std::string& filename, const std::string& access_type = "r");
    virtual ~archive();

    bool close();

  private:
    std::string _filename;
  };

}  // namespace green::h5pp

#endif  // H5PP_ARCHIVE_H
