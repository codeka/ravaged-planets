#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/lua.h>
#include <framework/logging.h>

namespace fs = boost::filesystem;

namespace fw {

void l_log_debug(std::string const &msg);

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
  // first, clear out the default package.path. we want to restrict where we look
  // for scripts to just those areas that we control
  //luabind::object package = luabind::globals(_state)["package"];
 // if (package.is_valid()) {
  //  package["path"] = (fs::initial_path() / "?.lua").string();
 // }

  // add the log.debug() method
 // luabind::module(_state, "log")[luabind::def("debug", &l_log_debug)];
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

void l_log_debug(std::string const &msg) {
  debug << "LUA : " << msg << std::endl;
}

}
