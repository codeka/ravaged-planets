#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace fw::lua::impl {

// RAII helper that will pop an item from the stack in its destructor.
class PopStack {
public:
  // Construct a PopStack that will pop n items from the Lua stack when we are destructed.
  inline PopStack(lua_State *l, int n)
  : l_(l), n_(n) {
  }

  inline ~PopStack() {
    lua_pop(l_, n_);
  }

private:
  lua_State *l_;
  const int n_;
};

}
