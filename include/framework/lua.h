#pragma once

#include <string>
#include <boost/filesystem.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace fw {
class lua_table;

/**
 * This class represents the Lua context. It contains the state for a given "instance" of Lua, and holds references
 * to all the global objects, registrations and so on.
 */
class lua_context: boost::noncopyable {
private:
  lua_State *_state;
  std::string _last_error;

  void setup_state();

public:
  lua_context();
  ~lua_context();

  /** Sets the search pattern that Lua will use when searching for scripts via {@code require()}. */
  void set_search_pattern(std::string const &pattern);

  /** Loads a .lua script (and executes it immediately - you should have set up the context ready to go) */
  bool load_script(boost::filesystem::path const &filename);

  /** If something returns an error, this'll return a string version of the last error that occurred. */
  inline std::string get_last_error() const {
    return _last_error;
  }
};

}
