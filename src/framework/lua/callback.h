#pragma once

#include <framework/lua/base.h>
#include <framework/lua/reference.h>

namespace fw::lua {

// Callback represents a function in Lua that we can call. Typically we hold these to call events and timers.
class Callback {
public:
  // Constructs a new callback that references nil.
  Callback() {
  }

  // Construct a new callback from the value at the given stack index.
  Callback(lua_State* l, int stack_index) : l_(l), ref_(l, stack_index) {
    if (lua_type(l, stack_index) != LUA_TFUNCTION) {
      // TODO: this is an error!
    }
  }

  template<typename... Arg>
  inline void operator ()(Arg... args) {
    call(0, args...);
  }

private:
  lua_State* l_;
  // A reference to the function object itself.
  Reference ref_;

  inline void call(int num_args) {
    ref_.push();
    if (lua_pcall(l_, num_args, 0, 0) != 0) {
      // TODO: throw exception?
    }
  }

  template<typename T, typename... Arg>
  inline void call(int num_args, T arg, Arg... args) {
    push(arg);
    call(num_args + 1, args...);
  }
};

}  // namespace fw::lua
