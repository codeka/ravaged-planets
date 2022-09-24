#pragma once

#include <string>
#include <map>
#include <optional>

#include <framework/lua/method.h>
#include <framework/lua/value.h>

namespace fw::lua {

// Represents a piece of userdata that backs a C++ class of type T. You can add methods
// and whatnot to the Userdata type and they will be added to the metatable of the underlying
// userdata in Lua. In this way, you can "link" Lua to C++.
template<typename T>
class Userdata : public BaseValue<Userdata<T>> {
public:
  // When this constructor is called, we assume the userdata is already on the stack.
  Userdata(lua_State* l, T* object) : l_(l), index_(lua_gettop(l)), object_(object) {
  }

  ~Userdata() {
    lua_pop(l_, 1);
  }

  void push() const {
    lua_pushvalue(l_, index_);
  }

private:
  lua_State* l_;
  int index_;

  // This is a reference to the underlying C++ object that we refer to.
  T* object_;
};


}  // namespace fw::lua
