#pragma once

#include <map>
#include <string>

#include <framework/lua/method.h>
#include <framework/lua/value.h>

namespace fw::lua {

// Metatables are used in Lua to apply methods and things to *another* value. Typically, we'll
// use it to add methods to a Userdata<T> so that we can access a UserData<T> defined in C++
// as if it were a native Lua table with methods and whatnot.
template<typename Owner>
class TableBuilder {
public:
  TableBuilder() {
  }

  inline TableBuilder& method(std::string_view name, MethodCall<Owner> method) {
    methods_[std::string(name)] = method;
    return *this;
  }

  // Takes the table on the top of Lua stack and adds all our methods, etc to it.
  void build(lua_State* l) {
    // Create the __index table
    lua_newtable(l);
    for (auto& entry : methods_) {
      lua_pushstring(l, entry.first.c_str());
      push(l, entry.second);
      lua_settable(l, -3);
    }
    // TODO: fields?

    // push the __index table into the metatable
    lua_pushstring(l, "__index");
    lua_pushvalue(l, -2); // the table that we already added to the stack
    lua_settable(l, -4);

    // The table we just set plus the __index table are no longer needed.
    lua_pop(l, 1);
  }

private:
  std::map<std::string, MethodCall<Owner>> methods_;
};

// Metatable<Owner> refers to a metatable for Userdata type Owner.
template<typename Owner>
class Metatable {
public:
  Metatable(const std::string& name, TableBuilder<Owner> builder) : name_(name), builder_(builder) {
  }

  // Pushes this metatable (possibly creating it if it doesn't already exist) onto the stack.
  void push(lua_State* l) {
    if (luaL_newmetatable(l, name_.c_str()) == 0) {
      // Already exists, we're done.
      return;
    }

    // Doesn't exist, but luaL_newmetatable has created the table, associated it with name_ and
    // pushed the new table onto the stack. We can populate it with the builder.
    builder_.build(l);
  }

private:
  std::string name_;
  TableBuilder<Owner> builder_;
};



}  // namespace fw::lua
