#pragma once

#include <framework/lua/push.h>
#include <framework/lua/userdata.h>

namespace fw::lua {

template<typename Owner>
class MethodContext {
public:
  MethodContext(lua_State* l, Userdata<Owner>* userdata) : l_(l), userdata_(userdata) {
  }

  /** Gets the "owner" of this method call. */
  Owner* owner() {
    return userdata_->owner();
  }

  template<typename T>
  T arg(int index) const {
    // We add 2 from the index. 1 because Lua indices start at 1 and 2 because the first element on the stack will be
    // the "this" pointer.
    return peek<T>(l_, index + 2);
  }

private:
  lua_State* l_;
  Userdata<Owner>* userdata_;
};

template<typename Owner>
using MethodCall = std::function<void(MethodContext<Owner>&)>;

// Contains the information we care about for a method call. Basically that means the
// std::function we *actually* call in response to a call from Lua.
template<typename Owner>
class MethodClosure {
public:
  MethodClosure(MethodCall<Owner> method) : method_(method) {
  }

  void call(lua_State* l, Userdata<Owner>* userdata) const {
    method_(MethodContext<Owner>(l, userdata));
  }

private:
  MethodCall<Owner> method_;
};

}  // namespace fw::lua
