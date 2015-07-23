#include <map>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/lua.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fs = boost::filesystem;

namespace fw {

namespace {
std::map<lua_State *, lua_context *> g_context_map;
}

class lua_debug {
public:
  static char const class_name[];
  static fw::lua_registrar<lua_debug>::method_definition methods[];

  int l_log(lua_context &ctx);
};

lua_context::lua_context() {
  _state = luaL_newstate();
  luaL_openlibs(_state);

  g_context_map[_state] = this;

  // sets up our custom functions and so on
  setup_state();
}

lua_context::~lua_context() {
  g_context_map.erase(_state);
  lua_close(_state);
}

/** Gets a reference to the lua_context for a given lua_State object. */
lua_context &lua_context::get(lua_State *state) {
  return *g_context_map[state];
}

void lua_context::setup_state() {
  // first, clear out the default package.path. we want to restrict where we look for scripts to just those areas that
  // we control
  set_search_pattern((fw::resolve("lua") / "?.lua").string());

  // add the 'debug' library.
  fw::lua_registrar<lua_debug>::register_static(_state, new lua_debug());
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
lua_callback::lua_callback(lua_State *state, int ref) :
  _state(state), _ref(ref) {
}

void lua_callback::call() {
  lua_rawgeti(_state, LUA_REGISTRYINDEX, _ref);
  int ret = lua_pcall(_state, 0, 0, 0);
  if (ret != 0) {
    std::string err = lua_tostring(_state, -1);
    debug << boost::format("ERR: calling Lua callback: %1%") % err << std::endl;
  }
}

//-----------------------------------------------------------------------------

char const lua_debug::class_name[] = "debug";
fw::lua_registrar<lua_debug>::method_definition lua_debug::methods[] = {
  {"log", &lua_debug::l_log},
  {nullptr, nullptr}
};

int lua_debug::l_log(lua_context &ctx) {
  char const *msg = luaL_checkstring(ctx.get_state(), 1);
  fw::debug << "LUA : " << msg << std::endl;
  return 1;
}

}
