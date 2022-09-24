#pragma once

#include <framework/lua/push.h>

namespace fw::lua {

template<typename Owner>
class MethodContext {
public:
  MethodContext(lua_State* l) : l_(l) {
    owner_ = peek_userdata<Owner*>(l_, 1);
  }

  /** Gets the "owner" of this method call. */
  Owner* owner() {
    return reinterpret_cast<Owner*>(owner_);
  }

  template<typename T>
  T arg(int index) const {
    // We subtract 2 from the index. 1 because Lua indices start at 1 and
    // 2 because the first element on the stack will be the "this" pointer.
    return peek<T>(l_, index - 1);
  }

private:
  lua_State* l_;
  Owner* owner_;
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

  void call(lua_State* l) {
    method_(MethodContext<Owner>(l));
  }

private:
  MethodCall<Owner> method_;
};

// All our calls from Lua start here. We grab the MethodClosure from the first
// upvalue and pass control to it.
template<typename Owner>
inline int lua_callback_(lua_State* l) {
  MethodClosure<Owner>* closure =
      reinterpret_cast<MethodClosure<Owner>*>(lua_touserdata(l, lua_upvalueindex(1)));
  closure->call(l);
  return 0; // TODO: how many?
}

template<typename Owner>
inline void push(lua_State* l, MethodCall<Owner> method) {
  MethodClosure<Owner>* closure =
      new(lua_newuserdata(l, sizeof(MethodClosure<Owner>))) MethodClosure<Owner>(method);

  // We have to pass the address of *some* implementation of lua_callback_, this is rather
  // hacky but seems to work (probably undefined behvior though :/)
  lua_pushcclosure(l, &lua_callback_<void>, 1);
}

}  // namespace fw::lua
