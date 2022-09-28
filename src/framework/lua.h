#pragma once

#include <memory>
#include <string>
#include <boost/filesystem.hpp>

#include <framework/lua/base.h>
#include <framework/lua/metatable.h>
#include <framework/lua/method.h>
#include <framework/lua/userdata.h>
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

  // Creates a brand new table, adds it to the registry. You can add your methods, fields etc to this table
  // then make it a global or pass it to a function or whatever you need.
  Value create_table();

  // Wrap the given object in a Userdata and return it so that you can push it onto the stack,
  // assign it to a global or whatever. The reference type T must have a public static field
  // named lua_registry_entry in order for us to wrap it.
  template<typename T>
  inline Value wrap(T* object) {
    Userdata<T>* userdata = new(lua_newuserdata(l_, sizeof(Userdata<T>))) Userdata<T>(l_, object);
    impl::PopStack pop(l_, 1);

    Metatable<T>& metatable = T::metatable;
    metatable.push(l_);

    lua_setmetatable(l_, -2);

    return Value(l_, -1);
  }

  // If something returns an error, this'll return a string version of the last error that occurred.
  std::string get_last_error() const { return last_error_; }
};

}

// TODO: delete this
namespace luabind {
struct object {};
}
