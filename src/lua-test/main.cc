#include <iostream>

#include <boost/exception/all.hpp>

#include <framework/lua.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/status.h>

fw::Status settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

//-----------------------------------------------------------------------------

/*
class UnitWrapper {
private:
  int l_attack(fw::lua::MethodContext &ctx);

  void attack(std::string const &msg);
public:
  static char const class_name[];
  static fw::lua_registrar<unit_wrapper>::method_definition methods[];

  UnitWrapper();
};

char const UnitWrapper::class_name[] = "Unit";
fw::lua_registrar<unit_wrapper>::method_definition unit_wrapper::methods[] = {
  {"attack", &unit_wrapper::l_attack},
  {nullptr, nullptr}
};

UnitWrapper::UnitWrapper() {
}

int UnitWrapper::l_attack(fw::lua::MethodContext& ctx) {
  char const *msg = luaL_checkstring(ctx.get_state(), 1);
  attack(msg);
  return 1;
}

void UnitWrapper::attack(std::string const &msg) {
  fw::debug << "ATTACK: " << msg << std::endl;
}
*/
/*
class ai_player {
private:
  int l_say(fw::lua_context& ctx);
  int l_find(fw::lua_context& ctx);
  int l_timer(fw::lua_context& ctx);

  void say(std::string const& msg);
  std::vector<unit_wrapper*> find();
public:
  static char const class_name[];
  static fw::lua_registrar<ai_player>::method_definition methods[];
};

char const ai_player::class_name[] = "Player";
fw::lua_registrar<ai_player>::method_definition ai_player::methods[] = {
  {"say", &ai_player::l_say},
  {"find", &ai_player::l_find},
  {"Timer", &ai_player::l_timer},
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

class TestClass {
private:
  static void l_debug(fw::lua::MethodContext<TestClass>& ctx) {
    ctx.owner()->debug(ctx.arg<std::string>(0));
  }

  static void l_register(fw::lua::MethodContext<TestClass>& ctx) {
    //ctx.owner()->callback = ctx.arg<fw::lua::Callback>(0);
  }

public:
  void debug(std::string msg) {
    fw::debug << "debug " << n << std::endl;
    fw::debug << msg << std::endl;
  }

  int n = 0;

  fw::lua::Callback callback;

  LUA_DECLARE_METATABLE(TestClass);
};

LUA_DEFINE_METATABLE(TestClass)
    .method("debug", TestClass::l_debug)
    .method("register", TestClass::l_register);

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    auto status = settings_initialize(argc, argv);
    if (!status.ok()) {
      std::cerr << status << std::endl;
      fw::Settings::print_help();
      return 1;
    }

    fw::ToolApplication app;
    new fw::Framework(&app);
    auto continue_or_status = fw::Framework::get_instance()->initialize("Lua Test");
    if (!continue_or_status.ok()) {
      fw::debug << continue_or_status.status() << std::endl;
      return 1;
    }
    if (!continue_or_status.value()) {
      return 0;
    }

    fw::lua::LuaContext ctx;

    TestClass test_class;


    ctx.globals()["foo"] = "bar";
    ctx.globals()["baz"] = 123;

    fw::lua::Value obj = ctx.create_table();
    obj["test"] = "I passed the test!";
    obj["test2"] = "and I will go into the West and remain Galadriel";
    ctx.globals()["test"] = obj;

    ctx.globals()["test_class"] = ctx.wrap(&test_class);
    test_class.n = 200;

    ctx.add_path("D:\\src\\ravaged-planets\\lua");
    ctx.load_script("D:\\src\\ravaged-planets\\lua\\main.lua");

    fw::lua::Value blah = ctx.globals()["Blah"];
    for (auto& kvp : blah) {
      fw::debug << "key/value " << kvp.key<std::string>() << " - " << kvp.value<fw::lua::Value>() << std::endl;
    }
    fw::debug << " Blah.name = " << blah["name"].debug_string() << std::endl;
    blah["desc"] = "hrm";
    for (auto& kvp : blah) {
      fw::debug << "key/value " << kvp.key<std::string>() << " - " << kvp.value<fw::lua::Value>() << std::endl;
    }

    test_class.callback();

/*

    fw::lua_registrar<unit_wrapper>::register_without_constructor(ctx.get_state());
    fw::lua_registrar<ai_player>::register_static(ctx.get_state(), "player", new ai_player());

    if (g_callback) {
      fw::debug << "g_callback is non-null, calling it now." << std::endl;
      g_callback->call();
    }
*/
  } catch(std::exception &e) {
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << e.what() << std::endl;

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
  ss << fw::LogFileName() << std::endl;
  ss << std::endl;
  ss << msg;
}

fw::Status settings_initialize(int argc, char** argv) {
  fw::SettingDefinition extra_settings;

  return fw::Settings::initialize(extra_settings, argc, argv, "lua-test.conf");
}
