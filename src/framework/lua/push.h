#pragma once

#include <string>
#include <string_view>

#include <framework/lua/base.h>
#include <framework/lua/callback.h>

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

template<>
inline float peek(lua_State* l, int index) {
  lua_Number n = lua_tonumber(l, index);
  return static_cast<float>(n);
}

template<>
inline int peek(lua_State* l, int index) {
  lua_Number n = lua_tonumber(l, index);
  return static_cast<int>(n);
}

// TODO: add more specializations as needed.

}
