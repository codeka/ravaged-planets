#include <map>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/lua.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fs = boost::filesystem;

namespace fw::lua {

void l_log_debug(std::string const &msg);

LuaContext::LuaContext() {
  l_ = luaL_newstate();
  luaL_openlibs(l_);
//  luabind::open(_state);

  // sets up our custom functions and so on
  setup_state();
}

LuaContext::~LuaContext() {
  lua_close(l_);
}

void LuaContext::setup_state() {
  // first, clear out the default package.path. we want to restrict where we look
  // for scripts to just those areas that we control
//  luabind::object package = luabind::globals(_state)["package"];
//  if (package.is_valid()) {
//    package["path"] = "";
//  }

  // add the log.debug() method
//  luabind::module(_state, "log")
//    [
//      luabind::def("debug", &l_log_debug)
//    ];
}

void LuaContext::add_path(fs::path const &path) {
//  luabind::object package = luabind::globals(_state)["package"];
//  if (package.is_valid()) {
//    // simply append the new path onto package.path
//    package["path"] = boost::lexical_cast<std::string>(package["path"]) + ";" + path.string();
//  }
}

bool LuaContext::load_script(fs::path const &filename) {
  last_error_ = "";
  debug << boost::format("loading script: %1%") % filename << std::endl;

  int ret = luaL_loadfile(l_, filename.string().c_str());
  if (ret != 0) {
    last_error_ = lua_tostring(l_, -1);
    debug << boost::format("ERR: could not load Lua script %1%:\n%2%")
        % filename % last_error_ << std::endl;
    return false;
  }

  ret = lua_pcall(l_, 0, 0, 0);
  if (ret != 0) {
    last_error_ = lua_tostring(_state, -1);
    debug << boost::format("ERR: could not load Lua script %1%:\n%2%")
        % filename % last_error_ << std::endl;
    return false;
  }

  return true;
}

Value LuaContext::globals() {
  lua_pushvalue(l_, LUA_GLOBALSINDEX);
  impl::PopStack pop(l_, 1);

  return Value(l_, -1);
}

//-------------------------------------------------------------------------

void l_log_debug(const std::string &msg) {
  debug << "LUA : " << msg << std::endl;
}

}
