#include <iostream>

#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>

#include <framework/lua.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace po = boost::program_options;

void settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

//-----------------------------------------------------------------------------
class unit_wrapper {

};

class ai_player {
private:
  int l_say(lua_State *state);
  int l_find(lua_State *state);

  void say(std::string const &msg);
  std::vector<unit_wrapper> find();
public:
  static char const class_name[];
  static fw::lua_registrar<ai_player>::method_definition methods[];

  ai_player(lua_State *state);
};

char const ai_player::class_name[] = "self";
fw::lua_registrar<ai_player>::method_definition ai_player::methods[] = {
  {"say", &ai_player::l_say},
  {"find", &ai_player::l_find},
  {nullptr, nullptr}
};

ai_player::ai_player(lua_State *state) {

}

int ai_player::l_say(lua_State *state) {
  char const *msg = luaL_checkstring(state, 1);
  say(msg);
  return 1;
}

int ai_player::l_find(lua_State *state) {
  return 1;
}

void ai_player::say(std::string const &msg) {
  fw::debug << "SAY: " << msg << std::endl;
}

std::vector<unit_wrapper> ai_player::find() {
  std::vector<unit_wrapper> units;
  return units;
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    fw::tool_application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Lua Test");

    fw::lua_context ctx;
    fw::lua_registrar<ai_player>::register_with_constructor(ctx.get_state());
    ctx.set_search_pattern("/Users/deanh/Downloads/?.lua");
    ctx.load_script("/Users/deanh/Downloads/main.lua");

  } catch(std::exception &e) {
    std::string msg = boost::diagnostic_information(e);
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << msg << std::endl;

    display_exception(e.what());
  } catch (...) {
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION! (unknown exception)" << std::endl;
  }

  return 0;
}

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::debug.get_filename() << std::endl;
  ss << std::endl;
  ss << msg;
}

void settings_initialize(int argc, char** argv) {
  po::options_description options("Additional options");

  fw::settings::initialize(options, argc, argv, "lua-test.conf");
}
