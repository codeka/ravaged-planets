#include <filesystem>
#include <map>

#include <boost/lexical_cast.hpp>

#include <framework/lua.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fs = std::filesystem;

namespace fw::lua {

namespace {

class LogWrapper {
private:
  static void l_debug(fw::lua::MethodContext<LogWrapper>& ctx) {
    ctx.owner()->debug(ctx.arg<std::string>(0));
  }

public:
  void debug(std::string msg) {
    fw::debug << msg << std::endl;
  }

  LUA_DECLARE_METATABLE(LogWrapper);
};

LUA_DEFINE_METATABLE(LogWrapper)
    .method("debug", LogWrapper::l_debug);

LogWrapper log_wrapper;

}

LuaContext::LuaContext() {
  l_ = luaL_newstate();
  luaL_openlibs(l_);

  // sets up our custom functions and so on
  setup_state();
}

LuaContext::~LuaContext() {
  lua_close(l_);
}

void LuaContext::setup_state() {
  // first, clear out the default package.path. we want to restrict where we look for scripts to just those areas that
  // we control.
  fw::lua::Value package = globals()["package"];
  if (!package.is_nil()) {
    package["path"] = "";
  }

  // add the log.debug() method
  globals()["log"] = wrap(&log_wrapper);
}

void LuaContext::add_path(fs::path const &path) {
  fw::lua::Value package = globals()["package"];
  if (!package.is_nil()) {
    // simply append the new path onto package.path
    package["path"] = std::string(package["path"]) + ";" + path.string();
  }
}

bool LuaContext::load_script(fs::path const &filename) {
  last_error_ = "";
  debug << "loading script: " << filename << std::endl;

  int ret = luaL_loadfile(l_, filename.string().c_str());
  if (ret != 0) {
    last_error_ = lua_tostring(l_, -1);
    debug << "ERR: could not load Lua script " << filename << ":\n" << last_error_ << std::endl;
    return false;
  }

  ret = lua_pcall(l_, 0, 0, 0);
  if (ret != 0) {
    last_error_ = lua_tostring(l_, -1);
    debug << "ERR: could not load Lua script " << filename << ":\n" << last_error_ << std::endl;
    return false;
  }

  return true;
}

Value LuaContext::globals() {
  lua_pushglobaltable(l_);
  impl::PopStack pop(l_, 1);

  return Value(l_, -1);
}

Value LuaContext::create_table() {
  lua_newtable(l_);
  impl::PopStack pop(l_, 1);

  return Value(l_, -1);
}

//-------------------------------------------------------------------------

void l_log_debug(const std::string &msg) {
  debug << "LUA : " << msg << std::endl;
}

}
