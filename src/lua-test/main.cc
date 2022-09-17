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
/*
std::shared_ptr<fw::lua_callback> g_callback;

//-----------------------------------------------------------------------------
class unit_wrapper {
private:
  int l_attack(fw::lua_context &ctx);

  void attack(std::string const &msg);
public:
  static char const class_name[];
  static fw::lua_registrar<unit_wrapper>::method_definition methods[];

  unit_wrapper();
};

class ai_player {
private:
  int l_say(fw::lua_context &ctx);
  int l_find(fw::lua_context &ctx);
  int l_timer(fw::lua_context &ctx);

  void say(std::string const &msg);
  std::vector<unit_wrapper *> find();
public:
  static char const class_name[];
  static fw::lua_registrar<ai_player>::method_definition methods[];
};

char const unit_wrapper::class_name[] = "Unit";
fw::lua_registrar<unit_wrapper>::method_definition unit_wrapper::methods[] = {
  {"attack", &unit_wrapper::l_attack},
  {nullptr, nullptr}
};

unit_wrapper::unit_wrapper() {
}

int unit_wrapper::l_attack(fw::lua_context &ctx) {
  char const *msg = luaL_checkstring(ctx.get_state(), 1);
  attack(msg);
  return 1;
}

void unit_wrapper::attack(std::string const &msg) {
  fw::debug << "ATTACK: " << msg << std::endl;
}

char const ai_player::class_name[] = "Player";
fw::lua_registrar<ai_player>::method_definition ai_player::methods[] = {
  {"say", &ai_player::l_say},
  {"find", &ai_player::l_find},
  {"timer", &ai_player::l_timer},
  {nullptr, nullptr}
};

int ai_player::l_say(fw::lua_context &ctx) {
  char const *msg = luaL_checkstring(ctx.get_state(), 1);
  say(msg);
  return 1;
}

int ai_player::l_find(fw::lua_context &ctx) {
  std::vector<unit_wrapper *> units(find());
  fw::lua_helper<unit_wrapper>::push(ctx.get_state(), units.begin(), units.end(), true);
  return 1;
}

int ai_player::l_timer(fw::lua_context &ctx) {
  double time = luaL_checknumber(ctx.get_state(), 1);
  g_callback = fw::lua_helper<ai_player>::check_callback(ctx.get_state(), 2);
  return 0;
}

void ai_player::say(std::string const &msg) {
  fw::debug << "SAY: " << msg << std::endl;
}

std::vector<unit_wrapper *> ai_player::find() {
  std::vector<unit_wrapper *> units;
  units.push_back(new unit_wrapper());
  units.push_back(new unit_wrapper());
  units.push_back(new unit_wrapper());
  return units;
}
*/
//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    fw::tool_application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Lua Test");

    fw::lua::LuaContext ctx;

    ctx.globals();

    ctx.add_path("D:\\src\\ravaged-planets\\lua");
    ctx.load_script("D:\\src\\ravaged-planets\\lua\\main.lua");


/*

    fw::lua_registrar<unit_wrapper>::register_without_constructor(ctx.get_state());
    fw::lua_registrar<ai_player>::register_static(ctx.get_state(), "player", new ai_player());

    if (g_callback) {
      fw::debug << "g_callback is non-null, calling it now." << std::endl;
      g_callback->call();
    }
*/
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
