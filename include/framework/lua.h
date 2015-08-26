#pragma once

#include <memory>
#include <string>
#include <boost/filesystem.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace fw {

/**
 * This class represents the Lua context. It's the main object you'll create when creating an interface to Lua
 * and it allows you to call scripts, register objects, functions and callbacks and so on.
 */
class lua_context : boost::noncopyable {
private:
  lua_State *_state;
  std::string _last_error;

  void setup_state();

public:
  lua_context();
  ~lua_context();

  /** Adds a path to the package.path that LUA uses to search for modules reference in require(...) statements. */
  void add_path(boost::filesystem::path const &path);

  /** Loads a .lua script (and executes it immediately - you should have set up the context ready to go). */
  bool load_script(boost::filesystem::path const &filename);

  /** You can pass a lua_context as a lua_State to luabind, for example. */
  operator lua_State *() { return _state; }

  /** If something returns an error, this'll return a string version of the last error that occurred. */
  std::string get_last_error() const { return _last_error; }
};

}
