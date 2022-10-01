#pragma once

#include <iostream>

#include <framework/lua/base.h>

namespace fw::lua {

// Defines a reference to an Lua value. Because Lua can rearrange objects and do garbage collection
// and whatnot, we need to refer to values using a special "reference" system, that this class
// wraps.
// See: https://www.lua.org/pil/27.3.2.html
class Reference {
public:
  // Creates a reference to the nil value.
  inline Reference() : l_(nullptr), ref_(LUA_REFNIL) {
  }

  // Create a reference to a value on the current Lua stack.
  inline Reference(lua_State* l, int stack_index) : l_(l) {
    lua_pushvalue(l_, stack_index);
    ref_ = luaL_ref(l_, LUA_REGISTRYINDEX);
  }

  inline Reference(const Reference& copy)
      : l_(copy.l_), ref_(LUA_REFNIL) {
    if (l_ != nullptr) {
      lua_rawgeti(l_, LUA_REGISTRYINDEX, copy.ref_);
      ref_ = luaL_ref(l_, LUA_REGISTRYINDEX);
    }
  }

  inline ~Reference() {
    if (l_ != nullptr && ref_ != LUA_REFNIL && ref_ != LUA_NOREF) {
      luaL_unref(l_, LUA_REGISTRYINDEX, ref_);
    }
  }

  bool is_nil() const {
    return l_ == nullptr || ref_ == LUA_REFNIL;
  }

  inline void swap(Reference& other) {
    std::swap(l_, other.l_);
    std::swap(ref_, other.ref_);
  }

  inline Reference& operator=(const Reference& other) {
    if (ref_ != LUA_REFNIL) {
      luaL_unref(l_, LUA_REGISTRYINDEX, ref_);
    }

    l_ = other.l_;
    if (l_ != nullptr) {
      lua_rawgeti(l_, LUA_REGISTRYINDEX, other.ref_);
      ref_ = luaL_ref(l_, LUA_REGISTRYINDEX);
    }

    return *this;
  }

  // Push this reference onto the stack so we can use it.
  void push() const {
    if (l_ == nullptr || ref_ == LUA_REFNIL) {
      // TODO: push nil
    } else {
      lua_rawgeti(l_, LUA_REGISTRYINDEX, ref_);
    }
  }

  lua_State* l() {
    return l_;
  }

private:
  friend std::ostream& operator<<(std::ostream& os, const Reference& r);
  mutable lua_State* l_;
  int ref_;
};

inline std::ostream& operator<<(std::ostream& os, const Reference& r) {
  r.push();
  impl::PopStack pop(r.l_, 1);

  os << "[ref " << r.ref_ << " " << lua_typename(r.l_, lua_type(r.l_, -1)) << "]";
  return os;
}

}
