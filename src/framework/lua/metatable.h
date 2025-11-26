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
  void build(lua_State* l);

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

  // If we have our own metatable, query that since we didn't otherwise find what we're looking for.
  if (lua_getmetatable(l, -2) != 0) {
    lua_pushvalue(l, -2);
    lua_gettable(l, -2);
    // Remote the metatable
    lua_remove(l, -2);
    ctx.record_return_value();
  }
}

namespace impl {

// Searches the "inheritance" tree for a userdata with the given metatable attached. If the value at the given index
// is a table and not a userdata, we'll try to get that table's metatable and keep looking.
template<typename T>
Userdata<T>* find_userdata(lua_State* l, int index, std::string_view metatable_name) {
  switch (lua_type(l, index)) {
  case LUA_TTABLE:
    if (lua_getmetatable(l, index) != 0) {
      return find_userdata<T>(l, -1, metatable_name);
    }
    // TODO: error
    fw::debug << "  find_userdata called on table with no metatable, expected: " << metatable_name << std::endl;
    return nullptr;

  case LUA_TUSERDATA:
    return reinterpret_cast<Userdata<T>*>(luaL_checkudata(l, index, metatable_name.data()));

  default:
    // TODO: error
    fw::debug << "  find_userdata called on unknown type" << std::endl;
    return nullptr;
  }
}

template<typename Owner>
inline int callback_impl(lua_State* l, std::string name) {
  Userdata<Owner>* userdata = impl::find_userdata<Owner>(l, 1, name);
  if (userdata == nullptr) {
    fw::debug << "invalid call, userdata does not have matching metatable." << std::endl;
    return 0;
  }
  const MethodClosure<Owner>* closure
      = reinterpret_cast<const MethodClosure<Owner>*>(lua_topointer(l, lua_upvalueindex(1)));
  return closure->call(l, userdata);
}

}  // namespace impl

#define LUA_DECLARE_METATABLE(Owner) \
  static fw::lua::Metatable<Owner> metatable

#define LUA_DEFINE_METATABLE(Owner) \
  static int fw_lua_callback_ ## Owner (lua_State* l) { \
    return fw::lua::impl::callback_impl<Owner>(l, #Owner); \
  } \
  fw::lua::Metatable<Owner> Owner ::metatable = fw::lua::Metatable<Owner>(#Owner, &fw_lua_callback_ ## Owner)

}  // namespace fw::lua
