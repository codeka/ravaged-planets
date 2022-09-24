#pragma once

#include <string>
#include <string_view>

#include <framework/lua/base.h>

namespace fw::lua {

inline void push(lua_State* l, int n) {
  lua_pushnumber(l, n);
}

inline void push(lua_State* l, double n) {
  lua_pushnumber(l, n);
}

inline void push(lua_State* l, float n) {
  lua_pushnumber(l, n);
}

inline void push(lua_State* l, const std::string& str) {
  lua_pushlstring(l, str.data(), str.size());
}

inline void push(lua_State* l, const char *str) {
  lua_pushstring(l, str);
}

inline void push(lua_State* l, std::string_view str) {
  lua_pushlstring(l, str.data(), str.size());
}

template<typename T>
void push(lua_State* l, const T& t) {
  t.push();
}

template<typename T>
T peek(lua_State* l, int index);

template<>
inline std::string peek(lua_State* l, int index) {
  size_t length = 0;
  const char* str = lua_tolstring(l, index, &length);
  return std::string(str, length);
}

template<typename T>
T* peek_userdata(lua_State* l, int index) {
  return reinterpret_cast<T*>(lua_touserdata(l, index));
}

// TODO: add more specializations as needed.

}