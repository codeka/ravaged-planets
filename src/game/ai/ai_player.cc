#include <functional>
#include <boost/bind/arg.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <framework/lua.h>
#include <framework/logging.h>
#include <framework/paths.h>

#include <game/ai/ai_player.h>
#include <game/ai/unit_wrapper.h>
#include <game/simulation/simulation_thread.h>
#include <game/simulation/commands.h>
#include <game/simulation/orders.h>
#include <game/world/world.h>
#include <game/entities/entity_manager.h>
#include <game/entities/entity.h>
#include <game/entities/ownable_component.h>
#include <game/entities/orderable_component.h>

namespace fs = boost::filesystem;

namespace game {

char const ai_player::class_name[] = "player";
fw::lua_registrar<ai_player>::method_definition ai_player::methods[] = {
  {"set_ready", &ai_player::l_set_ready},
  {"say", &ai_player::l_say},
  {"local_say", &ai_player::l_local_say},
  {"timer", &ai_player::l_timer},
  {"event", &ai_player::l_event},
  {"find_units", &ai_player::l_find_units},
  {"issue_order", &ai_player::l_issue_order},
  {nullptr, nullptr}
};

ai_player::ai_player(std::string const &name, script_desc *desc, uint8_t player_no) {
  _script_desc = *desc;
  _user_name = name;
  _player_no = player_no;

  // we do one update of the update queue, to ensure it's ready to go
  _upd_queue.update();

  // create a new lua_context for our script, then add all of our global
  // functions and so on to it
  std::shared_ptr<fw::lua_context> script(new fw::lua_context());
  fw::lua_registrar<unit_wrapper>::register_without_constructor(script->get_state());
  fw::lua_registrar<ai_player>::register_static(script->get_state(), this);

  // add the ..\data\ai\common path to the search pattern, as well as the directory the script was loaded from.
  std::string search_pattern = (fw::install_base_path() / "ai/common/?.lua").string()
      + ";" + (desc->filename.parent_path() / "?.lua").string();
  script->set_search_pattern(search_pattern);

  if (!script->load_script(_script_desc.filename.string())) {
    _is_valid = false;
  } else {
    _is_valid = true;
    _script = script;
  }
}

ai_player::~ai_player() {
}

int ai_player::l_set_ready(fw::lua_context &ctx) {
  set_ready();
  return 1;
}

void ai_player::set_ready() {
  _is_ready_to_start = true;
}

int ai_player::l_say(fw::lua_context &ctx) {
  std::string msg = luaL_checkstring(ctx.get_state(), 1);
  say(msg);
  return 1;
}

void ai_player::say(std::string const &msg) {
  // just "say" whatever they told us to say.
  // todo: this should be a proper network call
  simulation_thread::get_instance()->sig_chat(_user_name, msg);
}

int ai_player::l_local_say(fw::lua_context &ctx) {
  std::string msg = luaL_checkstring(ctx.get_state(), 1);
  local_say(msg);
  return 1;
}

void ai_player::local_say(std::string const &msg) {
  // just "say" whatever they told us to say... (but just locally, it's for debugging your scripts, basically)
  simulation_thread::get_instance()->sig_chat(_user_name, msg);
}

int ai_player::l_timer(fw::lua_context &ctx) {
  double dt = luaL_checknumber(ctx.get_state(), 1);
  std::shared_ptr<fw::lua_callback> callback = fw::lua_helper<ai_player>::check_callback(ctx.get_state(), 2);
  timer(dt, callback);
  return 1;
}

void ai_player::timer(float dt, std::shared_ptr<fw::lua_callback> callback) {
  _upd_queue.push(dt, [callback]() { callback->call(); });
}

void ai_player::fire_event(std::string const &event_name, std::map<std::string, std::string> const &parameters) {
  // fires the given named event (which'll fire off all our lua callbacks)
  lua_event_map::iterator it = _event_map.find(event_name);
  if (it == _event_map.end())
    return;

//  luabind::object lua_params;
//  for (std::map<std::string, std::string>::const_iterator it = parameters.begin(); it != parameters.end(); ++it) {
//    lua_params[it->first] = it->second;
//  }

//  BOOST_FOREACH(luabind::object obj, it->second) {
//    try {
//      obj(event_name);
////    }
 //   catch(luabind::error &e) {
 //     luabind::object error_msg(luabind::from_stack(e.state(), -1));
 //     fw::debug << boost::format("An exception occurred firing a LUA event\n%1%\n%2%")
 //     % e.what() % error_msg << std::endl;
 //   }
//  }
}

int ai_player::l_event(fw::lua_context &ctx) {
  std::string event_name = luaL_checkstring(ctx.get_state(), 1);
  std::shared_ptr<fw::lua_callback> callback = fw::lua_helper<ai_player>::check_callback(ctx.get_state(), 2);
  event(event_name, callback);
  return 1;
}

void ai_player::event(std::string const &event_name, std::shared_ptr<fw::lua_callback> callback) {
  lua_event_map::iterator it = _event_map.find(event_name);
  if (it == _event_map.end()) {
    _event_map[event_name] = lua_event_map::mapped_type();
    it = _event_map.find(event_name);
  }

  it->second.push_back(callback);
}

// this is the predicate we pass to the entity manager for our l_findunits() implementation
struct findunits_predicate {
private:
  ai_player *_plyr;
  std::vector<uint8_t> _player_nos;
  std::string _unit_type;

public:
  inline findunits_predicate(ai_player *plyr, std::map<std::string, std::string> const &params) : _plyr(plyr) {
//    luabind::iterator end;
 //   for (luabind::iterator it(params); it != end; ++it) {
  //    try {
 //       std::string key = luabind::object_cast<std::string>(it.key());

 //       if (key == "players") {
  //        luabind::object value = *it;
  //        if (luabind::type(value) == LUA_TTABLE) {
   //         // if it's a table, we treat it as an array
   //         fw::debug << "TODO: specifying multiple player_nos is not implemented" << std::endl;
      //    } else {
     //       // if it's not a table, it should be an integer
     //       _player_nos.push_back(luabind::object_cast<uint8_t>(value));
     //     }
   //     } else if (key == "unit_type") {
   //       _unit_type = luabind::object_cast<std::string>(*it);
   //     } else {
   //       fw::debug << boost::format("WARN: unknown option for findunits: %1%") % key << std::endl;
   //     }
    //  } catch (luabind::cast_failed &) {
   //     fw::debug << "WARN: findunits was passed some invalid options, the returned units may not be what you wanted!"
   //         << std::endl;
   //   }
   // }

    // set up some defaults if they didn't get set already...
  //  if (_player_nos.size() == 0) {
  //    _player_nos.push_back(plyr->get_player_no());
  //  }
  }

  inline bool operator()(std::shared_ptr<ent::entity> const &ent) {
    // if it's not ownable, we don't care about it...
    ent::ownable_component *ownable = ent->get_component<ent::ownable_component>();
    if (ownable == nullptr)
      return false;

    // check that it's one of the players we've asked for
    bool wrong_player = true;
    BOOST_FOREACH(uint8_t player_no, _player_nos) {
      if (player_no == ownable->get_owner()->get_player_no()) {
        wrong_player = false;
        break;
      }
    }
    if (wrong_player)
      return false;

    // if we're looking for a specific unit_type, check the entity's name
    if (_unit_type != "") {
      if (_unit_type != ent->get_name())
        return false;
    }

    return true;
  }
};

int ai_player::l_find_units(fw::lua_context &ctx) {
  std::map<std::string, std::string> params;
  std::vector<unit_wrapper *> units(find_units(params));
  fw::lua_helper<unit_wrapper>::push(ctx.get_state(), units.begin(), units.end(), true);
  return 1;
}

// this is the "workhorse" of the AI function. it searches for all
// of the units which match the parameters given (note: because of the
// luabind::adopt policy, LUA takes ownership of the object we return)
std::vector<unit_wrapper *> ai_player::find_units(std::map<std::string, std::string> &params) {
  std::vector<unit_wrapper *> units;

  // create the predicate object that does the actual searching
  findunits_predicate pred(this, params);

  // do the actual search
  ent::entity_manager *entmgr = game::world::get_instance()->get_entity_manager();
  std::list<std::weak_ptr<ent::entity>> entities = entmgr->get_entities(pred);

  BOOST_FOREACH(std::weak_ptr<ent::entity> &wp, entities) {
    std::shared_ptr<ent::entity> ent = wp.lock();
    if (!ent) {
      continue;
    }

    // Find the entity's unit_wrapper (if it has one) and use it. If it doesn't have one yet, create a new
    // one and use that.
    ent::entity_attribute *attr = ent->get_attribute("ai_wrapper");
    if (attr == nullptr) {
      unit_wrapper *wrapper = create_wrapper(ent->get_name());
      wrapper->set_entity(wp);
      ent->add_attribute(ent::entity_attribute("ai_wrapper", wrapper));
      attr = ent->get_attribute("ai_wrapper");
    }

    units.push_back(attr->get_value<unit_wrapper *>());
  }

  return units;
}

int ai_player::l_issue_order(fw::lua_context &ctx) {
  return 1;
}

// issues the given orders to the given units. We assime that units is an array
// of unit_wrappers and orders is an object containing the parameters for the order.
void ai_player::issue_order(std::vector<unit_wrapper *> &units, std::map<std::string, std::string> &orders) {
  BOOST_FOREACH(unit_wrapper *unit, units) {
    issue_order(unit, orders);
  }
}

void ai_player::issue_order(unit_wrapper *unit, std::map<std::string, std::string> &orders) {
  std::shared_ptr<ent::entity> entity = unit->get_entity().lock();
  if (!entity)
    return;

  ent::orderable_component *orderable = entity->get_component<ent::orderable_component>();

  std::string order = orders["order"];
  if (order == "build") {
    fw::debug << "issuing \"build\" order to unit... " << std::endl;

    // todo: we should make this "generic"
    std::shared_ptr<build_order> order = create_order<build_order>();
    order->template_name = orders["build_unit"];
    orderable->issue_order(order);
  } else {
    fw::debug << "unknown order!" << std::endl;
  }
}

// creates a new unit_wrapper for the given entity type
unit_wrapper *ai_player::create_wrapper(std::string const &entity_name) {
//  unit_creator_map::iterator it = _unit_creator_map.find(entity_name);
 // if (it != _unit_creator_map.end() && it->second.is_valid()) {
 ///   return it->second()[luabind::adopt(luabind::result)];
 // }

  // if we don't have a specific wrapper for this unit type, just create a generic one and return that instead
  return new unit_wrapper();
}

void ai_player::update() {
  _upd_queue.update();
}

// this is called when our local player is ready to start the game
void ai_player::local_player_is_ready() {
}

void ai_player::world_loaded() {
  fw::vector start_loc;

  std::map<int, fw::vector>::iterator start_it = game::world::get_instance()->get_player_starts().find(_player_no);
  if (start_it == game::world::get_instance()->get_player_starts().end()) {
    fw::debug
        << boost::format("WARN: no player_start defined for player %1%, choosing random") % static_cast<int>(_player_no)
        << std::endl;
    start_loc = fw::vector(13.0f, 0, 13.0f); // <todo, random
  } else {
    start_loc = start_it->second;
  }

  // todo: we should create a "capital" or "HQ" or something building instead of "factory"
  std::shared_ptr<create_entity_command> cmd(create_command<create_entity_command>());
  cmd->template_name = "factory";
  cmd->initial_position = start_loc;
  simulation_thread::get_instance()->post_command(cmd);

  fire_event("game_started");
}

// This is called each simulation frame when we get all the commands from other players
void ai_player::post_commands(std::vector<std::shared_ptr<command>> &) {
}

}
