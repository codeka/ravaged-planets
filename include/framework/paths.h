#pragma once

#include <boost/filesystem.hpp>

namespace fw {

  // Base "user" path and base "installation" path
  boost::filesystem::path user_base_path();
  boost::filesystem::path install_base_path();

  // Resolves the given path by looking for it first in the user directory, then in the data path, and finally, in
  // the install path. If for_write is true, then the file is always created in the user directory.
  boost::filesystem::path resolve(std::string const &path, bool for_write = false);
}
