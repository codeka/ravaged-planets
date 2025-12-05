#pragma once

#include <framework/logging.h>
#include <framework/lua/base.h>
#include <framework/lua/error.h>
#include <framework/lua/push.h>
#include <framework/lua/reference.h>

namespace fw::lua {

// Callback represents a function in Lua that we can call. Typically we hold these to call events and timers.
class Callback {
public:
  // Constructs a new callback that references nil.
  Callback() : l_(nullptr) {
  }

  // Construct a new callback from the value at the given stack index.
  Callback(lua_State* l, int stack_index) : l_(l) {
  }

  Callback(lua_State* l) : l_(l) {
    if (lua_type(l, -1) != LUA_TFUNCTION) {
      LOG(ERR) << "Attempt to create a Callback with something that is not a function.";
      lua_pop(l_, 1);
      l_ = nullptr;
    }
  }

  ~Callback() {
    if (l_ != nullptr) {
      lua_pop(l_, 1);
    }
  }

  // As we just refer to a value on the stack, you cannot copy/assign us.
  Callback(const Callback&) = delete;
  Callback& operator=(const Callback&) = delete;

  template<typename... Arg>
  inline void operator ()(Arg... args) {
    if (l_ == nullptr) return;

    impl::push_error_handler(l_);
    impl::PopStack pop(l_, 1);

    lua_pushvalue(l_, -2); // Push the function on top of the error handler
    call(0, args...);
  }

private:
  lua_State* l_;

  inline void call(int num_args) {
    int err = lua_pcall(l_, num_args, 0, lua_gettop(l_) - num_args - 1);
    if (err != 0) {
      // TODO: throw exception?
      LOG(ERR) << "error calling callback with " << num_args << " arguments, err=" << err;

      // Lua will push an error message onto the stack in case of "regular" errors.
      if (err == LUA_ERRRUN) {
        size_t length = 0;
        const char* str = lua_tolstring(l_, -1, &length);
        std::string msg(str, length);
        LOG(ERR) << "  " << msg;
      }
    }
  }

  template<typename T, typename... Arg>
  inline void call(int num_args, T arg, Arg... args) {
    push(l_, arg);
    call(num_args + 1, args...);
  }
};

}  // namespace fw::lua
