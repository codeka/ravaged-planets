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
private:
  int l_attack(lua_State *state);

  void attack(std::string const &msg);
public:
  static char const class_name[];
  static fw::lua_registrar<unit_wrapper>::method_definition methods[];

  unit_wrapper();
};

class ai_player {
private:
  int l_say(lua_State *state);
  int l_find(lua_State *state);

  void say(std::string const &msg);
  std::shared_ptr<unit_wrapper> find();
public:
  static char const class_name[];
  static fw::lua_registrar<ai_player>::method_definition methods[];

  ai_player(lua_State *state);
};

char const unit_wrapper::class_name[] = "unit";
fw::lua_registrar<unit_wrapper>::method_definition unit_wrapper::methods[] = {
  {"attack", &unit_wrapper::l_attack},
  {nullptr, nullptr}
};

unit_wrapper::unit_wrapper() {
}

int unit_wrapper::l_attack(lua_State *state) {
  char const *msg = luaL_checkstring(state, 1);
  attack(msg);
  return 1;
}

void unit_wrapper::attack(std::string const &msg) {
  fw::debug << "ATTACK: " << msg << std::endl;
}

char const ai_player::class_name[] = "player";
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
  std::shared_ptr<unit_wrapper> unit(find());
  fw::lua_registrar<unit_wrapper>::push(state, unit.get(), false);
  return 1;
}

void ai_player::say(std::string const &msg) {
  fw::debug << "SAY: " << msg << std::endl;
}

std::shared_ptr<unit_wrapper> ai_player::find() {
  return std::shared_ptr<unit_wrapper>(new unit_wrapper());
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    fw::tool_application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Lua Test");

    fw::lua_context ctx;
    fw::lua_registrar<unit_wrapper>::register_without_constructor(ctx.get_state());
    fw::lua_registrar<ai_player>::register_static(ctx.get_state());
    ctx.set_search_pattern("/home/deanh/Downloads/?.lua");
    ctx.load_script("/home/deanh/Downloads/main.lua");

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
