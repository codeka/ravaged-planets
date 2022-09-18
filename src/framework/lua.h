#pragma once

#include <memory>
#include <string>
#include <boost/filesystem.hpp>

#include <framework/lua/base.h>
#include <framework/lua/value.h>

namespace fw::lua {

/**
  * This class represents the Lua context. It's the main object you'll create when creating an interface to Lua
  * and it allows you to call scripts, register objects, functions and callbacks and so on.
  */
class LuaContext {
private:
  lua_State* l_;
  std::string last_error_;

  void setup_state();

public:
  LuaContext();
  ~LuaContext();

  LuaContext(const LuaContext&) = delete;

  // Adds a path to the package.path that LUA uses to search for modules reference in require(...) statements.
  void add_path(boost::filesystem::path const& path);

  // Loads a .lua script (and executes it immediately - you should have set up the context ready to go).
  bool load_script(boost::filesystem::path const& filename);

  // Gets a reference to the globals.
  Value globals();

  // If something returns an error, this'll return a string version of the last error that occurred.
  std::string get_last_error() const { return last_error_; }
};

}

// TODO: delete this
namespace luabind {
struct object {};
}
