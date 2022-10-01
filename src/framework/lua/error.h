#pragma once

#include <framework/lua/base.h>

namespace fw::lua {

namespace impl {

// Called by Lua when there is an error in a script or pcall. We'll update the error message with filename/line number
// information, for easier debugging.
int handle_lua_error(lua_State* l);

// Push the handle_lua_error function.
void push_error_handler(lua_State* l);

}  // namespace impl

}  // namespace fw::lu
