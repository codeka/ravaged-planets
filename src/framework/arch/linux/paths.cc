
#include "config.h"
#include "paths.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

static fs::path g_user_data_path;

namespace fw {

  // Base "user" path and base "installation" path
  fs::path user_base_path() {
    if (g_user_data_path == fs::path()) {
      fs::path path = fs::path(getenv("HOME"));
      path = path / ".ravaged-planets";

      // make sure the directory exists as well
      if (!fs::exists(path)) {
        fs::create_directories(path);
      }

      g_user_data_path = path;
    }

    return g_user_data_path;
  }

  fs::path install_base_path() {
    return fs::path(INSTALL_PREFIX);
  }

  fs::path resolve(std::string const &path, bool for_write /*=false*/) {
    fs::path absolute_path(path);
    if (!absolute_path.is_absolute()) {
      absolute_path = user_base_path() / path;
      if (!for_write && !fs::exists(absolute_path)) {
        absolute_path = install_base_path() / path;
      }
    }

    return absolute_path;
  }
}
