#include <map>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <luabind/luabind.hpp>
#include <luabind/object.hpp>

#include <framework/lua.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fs = boost::filesystem;

namespace fw {

void l_log_debug(std::string const &msg);

lua_context::lua_context() {
  _state = luaL_newstate();
  luaL_openlibs(_state);
  luabind::open(_state);

  // sets up our custom functions and so on
  setup_state();
}

lua_context::~lua_context() {
  lua_close(_state);
}

void lua_context::setup_state() {
  // first, clear out the default package.path. we want to restrict where we look
  // for scripts to just those areas that we control
  luabind::object package = luabind::globals(_state)["package"];
  if (package.is_valid()) {
    package["path"] = "";
  }

  // add the log.debug() method
  luabind::module(_state, "log")
    [
      luabind::def("debug", &l_log_debug)
    ];
}

void lua_context::add_path(fs::path const &path) {
  luabind::object package = luabind::globals(_state)["package"];
  if (package.is_valid()) {
    // simply append the new path onto package.path
    package["path"] = boost::lexical_cast<std::string>(package["path"]) + ";" + path.string();
  }
}

bool lua_context::load_script(fs::path const &filename) {
  _last_error = "";
  debug << boost::format("loading script: \"%1%\"") % filename << std::endl;

  int ret = luaL_loadfile(_state, filename.string().c_str());
  if (ret != 0) {
    _last_error = lua_tostring(_state, -1);
    debug << boost::format("ERR: could not load Lua script \"%1%\":\n%2%")
        % filename % _last_error << std::endl;
    return false;
  }

  ret = lua_pcall(_state, 0, 0, 0);
  if (ret != 0) {
    _last_error = lua_tostring(_state, -1);
    debug << boost::format("ERR: could not load Lua script \"%1%\":\n%2%")
        % filename % _last_error << std::endl;
    return false;
  }

  return true;
}

//-------------------------------------------------------------------------

void l_log_debug(std::string const &msg) {
  debug << "LUA : " << msg << std::endl;
}

}
