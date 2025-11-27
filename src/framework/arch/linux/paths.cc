
#include <filesystem>
#include <string>

#include <framework/paths.h>
#include <framework/settings.h>

namespace fs = std::filesystem;

namespace fw {

static fs::path g_user_base_path;
static fs::path g_install_base_path;

/** Base "user" path is ~/.ravaged-planets and is where you can install your custom maps, etc. */
fs::path user_base_path() {
  if (g_user_base_path.empty()) {
    fs::path path = fs::path(getenv("HOME"));
    path = path / ".ravaged-planets";

    // make sure the directory exists as well
    if (!fs::exists(path)) {
      fs::create_directories(path);
    }

    g_user_base_path = path;
  }

  return g_user_base_path;
}

/**
 * Install path is (usually) the directory up from where the exectuable is (that is, the exectuable is usually in
 * the bin/ folder under the install base path). It can be overwritten on the command line with --data-path.
 */
fs::path install_base_path() {
  if (g_install_base_path.empty()) {
    std::string data_path = Settings::get<std::string>("data-path");
    if (!data_path.empty()) {
      g_install_base_path = data_path;
    } else {
      fs::path exe_path = Settings::get_executable_path();
      g_install_base_path = exe_path.parent_path().parent_path();
    }
  }
  return g_install_base_path;
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

bool is_hidden(fs::path const &path) {
  return path.filename().string()[0] == '.';
}

}
