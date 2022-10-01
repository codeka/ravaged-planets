#include <framework/lua/error.h>

namespace fw::lua {

namespace impl {

int handle_lua_error(lua_State* l) {
  size_t length;
  const char* msg = lua_tolstring(l, -1, &length);

  luaL_traceback(l, l, msg, 2);

  lua_remove(l, -2); // remove the msg from the stack, leaving only the result of the traceback

  return 1; // Just the traceback is on the stack.
}

void push_error_handler(lua_State* l) {
  lua_pushcfunction(l, handle_lua_error);
}

}  // namespace impl

}  // namespace fw::lua