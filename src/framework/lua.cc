#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/lua.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fs = boost::filesystem;

namespace fw {

int l_debug_openlib(lua_State *);
int l_debug_log(lua_State *);

lua_context::lua_context() {
  _state = luaL_newstate();
  luaL_openlibs(_state);

  // sets up our custom functions and so on
  setup_state();
}

lua_context::~lua_context() {
  lua_close(_state);
}

void lua_context::setup_state() {
  // first, clear out the default package.path. we want to restrict where we look for scripts to just those areas that
  // we control
  set_search_pattern((fw::resolve("lua") / "?.lua").string());

  // add the 'debug' library.
  luaL_requiref(_state, "debug", &l_debug_openlib, 1);
  lua_pop(_state, 1); // luaL_requiref leaves the library table on the stack
}

std::string get_string(lua_State *s, std::string const &name) {
  lua_pushstring(s, name.c_str());
  lua_gettable(s, -2);
  if (!lua_isstring(s, -1)) {
    return ""; // TODO: handle errors
  }
  std::string value = lua_tostring(s, -1);
  lua_pop(s, 1);  /* remove value */
  return value;
}

void put_string (lua_State *s, std::string const &name, std::string const &value) {
  lua_pushstring(s, name.c_str());
  lua_pushstring(s, value.c_str());
  lua_settable(s, -3);
}

void lua_context::set_search_pattern(std::string const &pattern) {
  lua_getglobal(_state, "package");
  if (lua_istable(_state, -1)) {
    put_string(_state, "path", pattern);
  }
  lua_pop(_state, 1);
}

bool lua_context::load_script(fs::path const &filename) {
  _last_error = "";
  debug << boost::format("loading script: %1%") % filename << std::endl;

  int ret = luaL_loadfile(_state, filename.string().c_str());
  if (ret != 0) {
    _last_error = lua_tostring(_state, -1);
    debug << boost::format("ERR: could not load Lua script %1%:\n%2%") % filename % _last_error << std::endl;
    return false;
  }

  ret = lua_pcall(_state, 0, 0, 0);
  if (ret != 0) {
    _last_error = lua_tostring(_state, -1);
    debug << boost::format("ERR: could not execute Lua script %1%:\n%2%") % filename % _last_error << std::endl;
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------

int l_debug_openlib(lua_State *state) {
  const luaL_Reg debug_lib[] = {
    { "log", &l_debug_log},
    { nullptr, nullptr }
  };
  luaL_newlib(state, debug_lib);

  return 1;
}

int l_debug_log(lua_State *state) {
  char const *msg = luaL_checkstring(state, 1);
  fw::debug << "LUA : " << msg << std::endl;
  return 0;
}

}
