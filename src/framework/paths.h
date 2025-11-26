#pragma once

#include <filesystem>

namespace fw {

  // Base "user" path and base "installation" path
  std::filesystem::path user_base_path();
  std::filesystem::path install_base_path();

  // Resolves the given path by looking for it first in the user directory, then in the data path, and finally, in
  // the install path. If for_write is true, then the file is always created in the user directory.
  std::filesystem::path resolve(std::string const &path, bool for_write = false);

  /** Determines whether the given file is 'hidden' according to this system's rules for hidden files. */
  bool is_hidden(std::filesystem::path const &path);
}
