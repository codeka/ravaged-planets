#pragma once

#include <functional>
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
   
  // A property is similar to a method except from the Lua script's point of view, you use it quite differently.
  // TODO: have separate set and get methods?
  inline Metatable& property(std::string_view name, PropertyCall<Owner> property) {
    properties_[std::string(name)] = property;
    return *this;
  }

  inline lua_CFunction callback() const {
    return callback_;
  }

  inline std::string name() const {
    return name_;
  }

  // Pushes this metatable (possibly creating it if it doesn't already exist) onto the stack.
  inline void push(lua_State* l) {
    if (luaL_newmetatable(l, name_.c_str()) == 0) {
      // Already exists, we're done.
      return;
    }

    // Doesn't exist, but luaL_newmetatable has created the table, associated it with name_ and
    // pushed the new table onto the stack. We can populate it with the builder.
    build(l);
  }
private:
  void Metatable<Owner>::build(lua_State* l);

  // This function is a callback that we push for the __index metamethod. We'll figure out the correct method or
  // property or whatever is being requested, and push that onto the stack so Lua can call it.
  void index(lua_State* l, MethodContext<Owner>& ctx);

  std::string name_;
  lua_CFunction callback_;
  std::map<std::string, MethodCall<Owner>> methods_;
  std::map<std::string, PropertyCall<Owner>> properties_;
};

template<typename Owner>
inline void Metatable<Owner>::build(lua_State* l) {
  // Create the __index table
  lua_newtable(l);

  // push the __index table into the metatable
  lua_pushstring(l, "__index");
  fw::lua::push<Owner>(l, std::bind(&Metatable<Owner>::index, this, l, std::placeholders::_1));
  lua_settable(l, -4);

  // The table we just set plus the __index table are no longer needed.
  lua_pop(l, 1);
}

template<typename Owner>
inline void Metatable<Owner>::index(lua_State* l, MethodContext<Owner>& ctx) {
  std::string key = ctx.arg<std::string>(0);
  lua_pop(l, 1); // pop the key
  
  for (auto& kvp : methods_) {
    if (kvp.first == key) {
      ctx.return_value(kvp.second);
      return;
    }
  }

  for (auto& kvp : properties_) {
    if (kvp.first == key) {
      // Call the property directly, it should push the return value on the stack for us.
      auto prop_ctx = PropertyContext(l, ctx.owner());
      kvp.second(prop_ctx);
      if (!prop_ctx.has_return_value()) {
        lua_pushnil(l);
      }
      ctx.record_return_value();
      return;
    }
  }
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
    return closure->call(l, userdata); \
  } \
  fw::lua::Metatable<Owner> Owner ::metatable = fw::lua::Metatable<Owner>(#Owner, &fw_lua_callback_ ## Owner)

}  // namespace fw::lua
