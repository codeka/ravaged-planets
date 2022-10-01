#pragma once

#include <framework/logging.h>
#include <framework/lua/base.h>
#include <framework/lua/error.h>
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
      fw::debug << "Attempt to create a Callback with something that is not a function." << std::endl;
      // TODO: this is an error!
    }
  }

  template<typename... Arg>
  inline void operator ()(Arg... args) {
    fw::debug << "call " << ref_ << "(";
    impl::push_error_handler(l_);
    impl::PopStack pop(l_, 1);

    ref_.push();
    call(0, args...);
  }

private:
  lua_State* l_;
  // A reference to the function object itself.
  Reference ref_;

  inline void call(int num_args) {
    fw::debug << ")" << std::endl;

    int err = lua_pcall(l_, num_args, 0, lua_gettop(l_) - num_args - 1);
    if (err != 0) {
      // TODO: throw exception?
      fw::debug << "Error calling callback with " << num_args << " arguments, err=" << err << std::endl;

      // Lua will push an error message onto the stack in case of "regular" errors.
      if (err == LUA_ERRRUN) {
        size_t length = 0;
        const char* str = lua_tolstring(l_, -1, &length);
        std::string msg(str, length);

        fw::debug << "  " << msg << std::endl;
      }
    }
  }

  template<typename T, typename... Arg>
  inline void call(int num_args, T arg, Arg... args) {
    if (num_args > 0) {
      fw::debug << ", ";
    }
    fw::debug << arg;

    push(l_, arg);
    call(num_args + 1, args...);
  }
};

}  // namespace fw::lua
