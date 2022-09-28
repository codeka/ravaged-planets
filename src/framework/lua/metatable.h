#pragma once

#include <map>
#include <string>

#include <framework/lua/method.h>
#include <framework/lua/userdata.h>
#include <framework/lua/value.h>

namespace fw::lua {


// Metatable<Owner> refers to a metatable for Userdata type Owner.
template<typename Owner>
class Metatable {
public:
  Metatable(const std::string& name, lua_CFunction callback) : name_(name), callback_(callback) {
  }

  inline Metatable& method(std::string_view name, MethodCall<Owner> method) {
    methods_[std::string(name)] = method;
    return *this;
  }

  lua_CFunction callback() const {
    return callback_;
  }

  // Pushes this metatable (possibly creating it if it doesn't already exist) onto the stack.
  void push(lua_State* l) {
    if (luaL_newmetatable(l, name_.c_str()) == 0) {
      // Already exists, we're done.
      return;
    }

    // Doesn't exist, but luaL_newmetatable has created the table, associated it with name_ and
    // pushed the new table onto the stack. We can populate it with the builder.
    build(l);
  }

private:
  inline void Metatable<Owner>::build(lua_State* l);

  std::string name_;
  lua_CFunction callback_;
  std::map<std::string, MethodCall<Owner>> methods_;
};

template<typename Owner>
inline void push_method(lua_State* l, MethodCall<Owner> method, Metatable<Owner>& metatable) {
  MethodClosure<Owner>* closure =
    new(lua_newuserdata(l, sizeof(MethodClosure<Owner>))) MethodClosure<Owner>(method);

  // We have to pass the address of *some* implementation of lua_callback_, this is rather
  // hacky but seems to work (probably undefined behvior though :/)
  lua_pushcclosure(l, metatable.callback(), 1);
}

template<typename Owner>
inline void Metatable<Owner>::build(lua_State* l) {
  // Create the __index table
  lua_newtable(l);
  for (auto& entry : methods_) {
    lua_pushstring(l, entry.first.c_str());
    push_method(l, entry.second, *this);
    lua_settable(l, -3);
  }
  // TODO: fields?

  // push a lightuserdata with a pointer to ourselves so that we can get ourself back from the Lua metatable
  lua_pushstring(l, "fw_metatable_instance");
  lua_pushlightuserdata(l, this);
  lua_settable(l, -3);

  // push the __index table into the metatable
  lua_pushstring(l, "__index");
  lua_pushvalue(l, -2); // the table that we already added to the stack
  lua_settable(l, -4);

  // The table we just set plus the __index table are no longer needed.
  lua_pop(l, 1);
}

#define LUA_DECLARE_METATABLE(Owner) \
  static fw::lua::Metatable<Owner> metatable

#define LUA_DEFINE_METATABLE(Owner) \
  static int fw_lua_callback_ ## Owner (lua_State* l) { \
    fw::lua::Userdata<Owner>* userdata = reinterpret_cast<fw::lua::Userdata<Owner>*>(luaL_checkudata(l, 1, #Owner)); \
    if (userdata == nullptr) { \
      fw::debug << "invalid call, userdata does not have matching metatable." << std::endl; \
      return 0; \
    } \
    const fw::lua::MethodClosure<Owner>* closure = \
        reinterpret_cast<const fw::lua::MethodClosure<Owner>*>(lua_topointer(l, lua_upvalueindex(1))); \
    closure->call(l, userdata); \
    fw::debug << "this is called here" << std::endl; \
    return 0; \
  } \
  fw::lua::Metatable<Owner> Owner ::metatable = fw::lua::Metatable<Owner>(#Owner, &fw_lua_callback_ ## Owner)

}  // namespace fw::lua
