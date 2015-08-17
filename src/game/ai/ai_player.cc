#include <functional>
#include <boost/bind/arg.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <luabind/luabind.hpp>
#include <luabind/raw_policy.hpp>
#include <luabind/adopt_policy.hpp>

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

ai_player::ai_player(std::string const &name, script_desc const &desc, uint8_t player_no) {
  _script_desc = desc;
  _user_name = name;
  _player_no = player_no;

  // we do one update of the update queue, to ensure it's ready to go
  _upd_queue.update();

  // create a new lua_context for our script, then add all of our global
  // functions and so on to it
  std::shared_ptr<fw::lua_context> script(new fw::lua_context());

  luabind::module(*script) [
      luabind::class_<ai_player>("ai_player")
          .def("set_ready", &ai_player::l_set_ready)
          .def("say", &ai_player::l_say)
          .def("local_say", &ai_player::l_local_say)
          .def("timer", &ai_player::l_timer)
          .def("event", &ai_player::l_event)
          .def("register_unit", &ai_player::l_register_unit)
          .def("find_units", &ai_player::l_find_units, luabind::raw(_3))
          .def("issue_order", &ai_player::l_issue_order)
  ];
  unit_wrapper::register_class(*script);
  luabind::globals(*script)["player"] = this;
  //player->self = luabind::get_globals(state)["player"];

  // add the ..\data\ai\common path to the package.path variable (so you can
  // just go require("whatever") to load stuff from there)
  script->add_path(fw::install_base_path() / "ai/common/?.lua");

  // also add the AI script's directory so we can pick up any extra scripts you might have defined
  script->add_path(_script_desc.filename.parent_path() / "?.lua");

  if (!script->load_script(_script_desc.filename.string())) {
    _is_valid = false;
  } else {
    _is_valid = true;
    _script = script;
  }
}

ai_player::~ai_player() {
}

void ai_player::l_set_ready() {
  _is_ready_to_start = true;
}

void ai_player::l_say(std::string const &msg) {
  // just "say" whatever they told us to say...
  // todo: this should be a proper network call
  simulation_thread::get_instance()->sig_chat(_user_name, msg);
}

void ai_player::l_local_say(std::string const &msg) {
  // just "say" whatever they told us to say... (but just locally, it's for debugging your scripts, basically)
  simulation_thread::get_instance()->sig_chat(_user_name, msg);
}

void ai_player::l_timer(float dt, luabind::object obj) {
  // this is called to queue a LUA function to our update_queue so we can call a Lua function at the given time
  if (!obj.is_valid())
    return;

  _upd_queue.push(dt, [obj]() mutable {
    fw::debug << "Calling: " << obj << std::endl;
    obj();
  });
}

void ai_player::fire_event(std::string const &event_name, std::map<std::string, std::string> const &parameters) {
  // fires the given named event (which'll fire off all our lua callbacks)
  lua_event_map::iterator it = _event_map.find(event_name);
  if (it == _event_map.end())
    return;

  luabind::object lua_params;
  for(auto it = parameters.begin(); it != parameters.end(); ++it) {
    lua_params[it->first] = it->second;
  }

  BOOST_FOREACH(luabind::object obj, it->second) {
    try {
      obj(event_name);
    } catch (luabind::error &e) {
      luabind::object error_msg(luabind::from_stack(e.state(), -1));
      fw::debug << boost::format("An exception occured executing a Lua event handler\n%1%\n%2%")
          % e.what() % error_msg << std::endl;
    }
  }
}

void ai_player::l_event(std::string const &event_name, luabind::object obj) {
  // this is called to queue a LUA function when the given named event occurs.
  if (!obj.is_valid())
    return;

  lua_event_map::iterator it = _event_map.find(event_name);
  if (it == _event_map.end()) {
    _event_map[event_name] = lua_event_map::mapped_type();
    it = _event_map.find(event_name);
  }

  it->second.push_back(obj);
}

// registers the given "creator" function that we'll use to create subclasses of unit_wrapper with
void ai_player::l_register_unit(std::string name, luabind::object creator) {
  _unit_creator_map[name] = creator;
}

// this is the predicate we pass to the entity manager for our l_findunits() implementation
struct findunits_predicate {
private:
  ai_player *_plyr;
  std::vector<uint8_t> _player_nos;
  std::string _unit_type;

public:
  inline findunits_predicate(ai_player *plyr, luabind::object &params) : _plyr(plyr) {
    luabind::iterator end;
    for(luabind::iterator it(params); it != end; ++it) {
      std::string key = luabind::object_cast<std::string>(it.key());

      if (key == "players") {
        luabind::object value = *it;
        if (luabind::type(value) == LUA_TTABLE) {
          // if it's a table, we treat it as an array
          fw::debug << "TODO: specifying multiple player_nos is not implemented" << std::endl;
        } else {
          // if it's not a table, it should be an integer
          _player_nos.push_back(luabind::object_cast<uint8_t>(value));
        }
      } else if (key == "unit_type") {
        _unit_type = luabind::object_cast<std::string>(*it);
      } else {
        fw::debug << boost::format("WARN: unknown option for findunits: %1%") % key << std::endl;
      }
    }

    // set up some defaults if they didn't get set already...
    if (_player_nos.size() == 0) {
      _player_nos.push_back(plyr->get_player_no());
    }
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
    if (wrong_player) {
      return false;
    }

    // if we're looking for a specific unit_type, check the entity's name
    if (_unit_type != "") {
      if (_unit_type != ent->get_name())
        return false;
    }

    return true;
  }
};

// this is the "workhorse" of the AI function. it searches for all
// of the units which match the parameters given (note: because of the
// luabind::adopt policy, LUA takes ownership of the object we return)
luabind::object ai_player::l_find_units(luabind::object params, lua_State *L) {
  luabind::object units = luabind::newtable(L);

  // if you pass something that's not a table as the first parameter,
  // we can't do anything so we just return an empty set.
  if (luabind::type(params) != LUA_TTABLE) {
    return units;
  }

  // create the predicate object that does the actual searching
  findunits_predicate pred(this, params);

  // do the actual search
  ent::entity_manager *entmgr = game::world::get_instance()->get_entity_manager();
  std::list<std::weak_ptr<ent::entity>> entities = entmgr->get_entities(pred);

  int index = 1;
  BOOST_FOREACH(std::weak_ptr<ent::entity> &wp, entities) {
    std::shared_ptr<ent::entity> ent = wp.lock();
    if (!ent) {
      continue;
    }

    // find the entity's unit_wrapper (if it has one) and return it. if it doesn't
    // have one yet, create a new one and return that.
    ent::entity_attribute *attr = ent->get_attribute("ai_wrapper");
    if (attr == nullptr) {
      luabind::object wrapper = create_wrapper(ent->get_name());
      luabind::object_cast<unit_wrapper *>(wrapper)->set_entity(wp);
      ent->add_attribute(ent::entity_attribute("ai_wrapper", wrapper));
      attr = ent->get_attribute("ai_wrapper");
    }

    units[index++] = attr->get_value<luabind::object>();
  }

  // and return the object
  return units;
}

// issues the given orders to the given units. We assime that units is an array
// of unit_wrappers and orders is an object containing the parameters for the order.
void ai_player::l_issue_order(luabind::object units, luabind::object orders) {
  // if you pass something that's not a table as the parameters,
  // we can't do anything.
  if (luabind::type(units) != LUA_TTABLE || luabind::type(orders) != LUA_TTABLE) {
    return;
  }

  boost::optional<unit_wrapper *> unit = luabind::object_cast_nothrow<unit_wrapper *>(units);
  if (unit) {
    // if they just passed one unit in, we'll just issue the order to that unit
    issue_order(*unit, orders);
  } else {
    luabind::iterator end;
    for(luabind::iterator it(units); it != end; ++it) {
      unit = luabind::object_cast_nothrow<unit_wrapper *>(*it);
      if (unit) {
        issue_order(*unit, orders);
      }
    }
  }
}

void ai_player::issue_order(unit_wrapper *unit, luabind::object orders) {
  std::shared_ptr<ent::entity> entity = unit->get_entity().lock();
  if (!entity)
    return;

  ent::orderable_component *orderable = entity->get_component<ent::orderable_component>();

  if (luabind::object_cast<std::string>(orders["order"]) == "build") {
    fw::debug << "issuing \"build\" order to unit... " << orders << std::endl;

    // todo: we should make this "generic"
    std::shared_ptr<build_order> order = create_order<build_order>();
    order->template_name = luabind::object_cast<std::string>(orders["build_unit"]);
    orderable->issue_order(order);
  } else {
    fw::debug << "unknown order!" << std::endl;
  }
}

// creates a new unit_wrapper for the given entity type
luabind::object ai_player::create_wrapper(std::string const &entity_name) {
  unit_creator_map::iterator it = _unit_creator_map.find(entity_name);
  if (it != _unit_creator_map.end() && it->second.is_valid()) {
    return it->second() [luabind::adopt(luabind::result)];
  }

  // if we don't have a specific wrapper for this unit type, just create a generic
  // one and return that instead
  return luabind::call_function<luabind::object>(*_script, "Unit");
}

void ai_player::update() {
  _upd_queue.update();
}

// this is called when our local player is ready to start the game
void ai_player::local_player_is_ready() {
}

void ai_player::world_loaded() {
  fw::vector start_loc;

  auto start_it = game::world::get_instance()->get_player_starts().find(_player_no);
  if (start_it == game::world::get_instance()->get_player_starts().end()) {
    fw::debug << boost::format("WARN: no player_start defined for player %1%, choosing random")
        % static_cast<int>(_player_no) << std::endl;
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
