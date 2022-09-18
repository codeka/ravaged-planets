#pragma once

#include <framework/lua/base.h>

namespace fw::lua {

// Defines a reference to an Lua value. Because Lua can rearrange objects and do garbage collection
// and whatnot, we need to refer to values using a special "reference" system, that this class
// wraps.
// See: https://www.lua.org/pil/27.3.2.html
class Reference {
public:
  // Creates a reference to the nil value.
  Reference();

  // Create a reference to a value on the current Lua stack.
  Reference(lua_State* l, int stack_index);

  Reference(const Reference& copy);
  Reference(const Reference&& move);

  ~Reference();

  void swap(Reference& other);
  Reference& operator=(Reference other);

  // Push this reference onto the stack so we can use it.
  void push() const;

  lua_State* l() {
    return l_;
  }

private:
  lua_State* l_;
  int ref_;
};




}