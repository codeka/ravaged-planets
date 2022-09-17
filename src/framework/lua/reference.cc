#include <framework/lua/reference.h>

#include <algorithm>

namespace fw::lua {

Reference::Reference()
: l_(nullptr), ref_(LUA_NOREF) {

}

Reference::Reference(lua_State* l, int stack_index)
: l_(l) {
  lua_pushvalue(l_, stack_index);
  ref_ = luaL_ref(l_, LUA_REGISTRYINDEX);
}

Reference::Reference(const Reference& copy)
: l_(copy.l_), ref_(LUA_NOREF) {
  if (l_ != nullptr) {
    lua_rawgeti(l_, LUA_REGISTRYINDEX, copy.ref_);
    ref_ = luaL_ref(l_, LUA_REGISTRYINDEX);
  }
}

Reference::Reference(const Reference&& move)
: l_(move.l_), ref_(move.ref_) {
}

Reference::~Reference() {
  if (l_ != nullptr) {
    luaL_unref(l_, LUA_REGISTRYINDEX, ref_);
  }
}

void Reference::swap(Reference& other) {
  std::swap(l_, other.l_);
  std::swap(ref_, other.ref_);
}

Reference& Reference::operator=(Reference other) {
  Reference(other).swap(*this);
  return *this;
}

void Reference::push() const {
  lua_rawgeti(l_, LUA_REGISTRYINDEX, ref_);
}

}
