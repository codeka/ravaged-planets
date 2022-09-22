#pragma once

#include <string>
#include <string_view>

#include <framework/lua/base.h>

namespace fw::lua {

template<typename T>
void push(lua_State* l, T t);

template<>
inline void push(lua_State* l, int n) {
  lua_pushnumber(l, n);
}

template<>
inline void push(lua_State* l, double n) {
  lua_pushnumber(l, n);
}

template<>
inline void push(lua_State* l, float n) {
  lua_pushnumber(l, n);
}

template<>
inline void push(lua_State* l, const std::string& str) {
  lua_pushlstring(l, str.data(), str.size());
}

template<>
inline void push(lua_State* l, const char* str) {
  lua_pushstring(l, str);
}

template<>
inline void push(lua_State* l, const std::string_view str) {
  lua_pushlstring(l, str.data(), str.size());
}

template<typename T>
void push(lua_State* l, T t) {
  t.push();
}

// TODO: add more specializations as needed.

}