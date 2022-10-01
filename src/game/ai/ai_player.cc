#include <functional>
#include <boost/bind/arg.hpp>
#include <boost/filesystem.hpp>

//#include <luabind/luabind.hpp>
//#include <luabind/raw_policy.hpp>
//#include <luabind/adopt_policy.hpp>

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

LUA_DEFINE_METATABLE(AIPlayer)
    .method("set_ready", AIPlayer::l_set_ready)
    .method("say", AIPlayer::l_say)
    .method("local_say", AIPlayer::l_local_say)
    .method("timer", AIPlayer::l_timer)
    .method("event", AIPlayer::l_event)
    .method("register_unit", AIPlayer::l_register_unit)
    .method("find_units", AIPlayer::l_find_units)
    .method("issue_order", AIPlayer::l_issue_order);


AIPlayer::AIPlayer(std::string const &name, ScriptDesc const &desc, uint8_t player_no) {
  script_desc_ = desc;
  user_name_ = name;
  player_no_ = player_no;

  // we do one update of the update queue, to ensure it's ready to go
  update_queue_.update();

  int color_index = static_cast<int>(fw::random() * player_colors.size());
  color_ = player_colors[color_index];

  // create a new lua_context for our script, then add all of our global
  // functions and so on to it
  std::shared_ptr<fw::lua::LuaContext> script(new fw::lua::LuaContext());

  script->globals()["player"] = script->wrap(this);
  //player->self = luabind::get_globals(state)["player"];

  // add the ..\data\ai\common path to the package.path variable (so you can
  // just go require("whatever") to load stuff from there)
  script->add_path(fw::install_base_path() / "ai/common/?.lua");

  // also add the AI script's directory so we can pick up any extra scripts you might have defined
  script->add_path(script_desc_.filename.parent_path() / "?.lua");

  if (!script->load_script(script_desc_.filename.string())) {
    is_valid_ = false;
  } else {
    is_valid_ = true;
    script_ = script;
  }
}

AIPlayer::~AIPlayer() {
}

/* static */
void AIPlayer::l_set_ready(fw::lua::MethodContext<AIPlayer>& ctx) {
  fw::debug << "setting AI player ready" << std::endl;
  ctx.owner()->is_ready_to_start_ = true;
}

/* static */
void AIPlayer::l_say(fw::lua::MethodContext<AIPlayer>& ctx) {
  // just "say" whatever they told us to say...
  // todo: this should be a proper network call
  SimulationThread::get_instance()->sig_chat(ctx.owner()->user_name_, ctx.arg<std::string>(0));
}

/* static */
void AIPlayer::l_local_say(fw::lua::MethodContext<AIPlayer>& ctx) {
  std::string msg = ctx.arg<std::string>(0);

  fw::debug << "SAY : " << msg << std::endl;
  // just "say" whatever they told us to say... (but just locally, it's for debugging your scripts, basically)
  SimulationThread::get_instance()->sig_chat(ctx.owner()->user_name_, msg);
}

/* static */
void AIPlayer::l_timer(fw::lua::MethodContext<AIPlayer>& ctx) {
  // this is called to queue a Lua function to our update_queue so we can call a Lua function at the given time
  float time = ctx.arg<float>(0);
  fw::lua::Callback fn = ctx.arg<fw::lua::Callback>(1);
  ctx.owner()->update_queue_.push(time, [fn]() mutable {
    fn();
  });
}

void AIPlayer::fire_event(std::string const &event_name, std::map<std::string, std::string> const &parameters) {
  // fires the given named event (which'll fire off all our lua callbacks)
  auto it = event_map_.find(event_name);
  if (it == event_map_.end())
    return;

  fw::lua::Value lua_params = script_->create_table();
  for(auto it = parameters.begin(); it != parameters.end(); ++it) {
    lua_params[it->first] = it->second;
  }

  for(auto& obj : it->second) {
    // TODO: handle errors gracefully?
    obj(event_name, lua_params);
  }
}

/* static */
void AIPlayer::l_event(fw::lua::MethodContext<AIPlayer>& ctx) {
  // this is called to queue a LUA function when the given named event occurs.
  std::string event_name = ctx.arg<std::string>(0);
  fw::lua::Callback event_callback = ctx.arg<fw::lua::Callback>(1);

  auto& event_map = ctx.owner()->event_map_;
  auto it = event_map.find(event_name);
  if (it == event_map.end()) {
    event_map[event_name] = LuaEventMap::mapped_type();
    it = event_map.find(event_name);
  }

  it->second.push_back(event_callback);
}

// registers the given "creator" function that we'll use to create subclasses of unit_wrapper with
/* static */
void AIPlayer::l_register_unit(fw::lua::MethodContext<AIPlayer>& ctx) {
  std::string name = ctx.arg<std::string>(0);
  fw::lua::Value creator_class = ctx.arg<fw::lua::Value>(1);
  ctx.owner()->unit_creator_map_[name] = creator_class;
}

// this is the predicate we pass to the Entity manager for our l_findunits() implementation
struct findunits_predicate {
private:
  AIPlayer *_plyr;
  std::vector<uint8_t> _player_nos;
  std::string _unit_type;
  std::string state_;

public:
  inline findunits_predicate(AIPlayer *plyr, luabind::object &params) : _plyr(plyr) {
//    luabind::iterator end;
/*    for (luabind::iterator it(params); it != end; ++it) {
      std::string key = luabind::object_cast<std::string>(it.key());

      if (key == "players" || key == "player") {
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
      } else if (key == "state") {
        _state = luabind::object_cast<std::string>(*it);
      } else {
        fw::debug << boost::format("WARN: unknown option for findunits: %1%") % key << std::endl;
      }
    }*/

    // set up some defaults if they didn't get set already...
    if (_player_nos.size() == 0) {
      _player_nos.push_back(plyr->get_player_no());
    }
  }

  inline bool operator()(std::shared_ptr<ent::Entity> const &ent) {
    // if it's not ownable, we don't care about it...
    ent::OwnableComponent *ownable = ent->get_component<ent::OwnableComponent>();
    if (ownable == nullptr)
      return false;

    // check that it's one of the players we've asked for
    bool wrong_player = true;
    for(uint8_t player_no : _player_nos) {
      if (player_no == ownable->get_owner()->get_player_no()) {
        wrong_player = false;
        break;
      }
    }
    if (wrong_player) {
      return false;
    }

    // if we're looking for a specific unit_type, check the Entity's name
    if (_unit_type != "") {
      if (_unit_type != ent->get_name()) {
        return false;
      }
    }

    // if we're looking for entities in a particular state, then check that
    if (state_ != "") {
      ent::OrderableComponent *orderable = ent->get_component<ent::OrderableComponent>();
      if (orderable == nullptr) {
        return false;
      }
      std::shared_ptr<game::Order> curr_order = orderable->get_current_order();
      if (curr_order && curr_order->get_state_name() != state_) {
        return false;
      } else if (!curr_order && state_ != "idle") {
        return false;
      }
    }

    return true;
  }
};

// this is the "workhorse" of the AI function. it searches for all
// of the units which match the parameters given (note: because of the
// luabind::adopt policy, LUA takes ownership of the object we return)
/* static */
void AIPlayer::l_find_units(fw::lua::MethodContext<AIPlayer>& ctx) {
//    luabind::object units;// = luabind::newtable(L);

  // if you pass something that's not a table as the first parameter,
  // we can't do anything so we just return an empty set.
//  if (luabind::type(params) != LUA_TTABLE) {
//    return units;
//  }

  // create the predicate object that does the actual searching
//  findunits_predicate pred(this, params);

  // do the actual search
//  ent::EntityManager *entmgr = game::World::get_instance()->get_entity_manager();
//  std::list<std::weak_ptr<ent::Entity>> entities = entmgr->get_entities(pred);

//  int index = 1;
//  for(std::weak_ptr<ent::Entity> &wp : entities) {
//    luabind::object wrapper = get_unit_wrapper(wp);
//    if (!wrapper) {
//      continue;
//    }
//    units[index++] = wrapper;
//  }

  // and return the object
//  return units;
}

// issues the given orders to the given units. We assime that units is an array
// of unit_wrappers and orders is an object containing the parameters for the order.
/* static */
void AIPlayer::l_issue_order(fw::lua::MethodContext<AIPlayer>& ctx) {
  // if you pass something that's not a table as the parameters, we can't do anything.
//  if (luabind::type(units) != LUA_TTABLE || luabind::type(orders) != LUA_TTABLE) {
//    return;
//  }

//    boost::optional<UnitWrapper*> unit;// = luabind::object_cast_nothrow<unit_wrapper*>(units);
//  if (unit) {
    // if they just passed one unit in, we'll just issue the order to that unit
//    issue_order(*unit, orders);
//  } else {
//    luabind::iterator end;
//    for(luabind::iterator it(units); it != end; ++it) {
//      unit = luabind::object_cast_nothrow<unit_wrapper *>(*it);
//      if (unit) {
//        issue_order(*unit, orders);
//      }
//    }
//  }
}

void AIPlayer::issue_order(UnitWrapper *unit, luabind::object orders) {
  std::shared_ptr<ent::Entity> Entity = unit->get_entity().lock();
  if (!Entity)
    return;

  ent::OrderableComponent *orderable = Entity->get_component<ent::OrderableComponent>();

  std::string order_name;// = luabind::object_cast<std::string>(orders["order"]);
  if (order_name == "build") {
    fw::debug << "issuing \"build\" order to unit." << std::endl;

    // todo: we should make this "generic"
    std::shared_ptr<BuildOrder> order = create_order<BuildOrder>();
    order->template_name;// = luabind::object_cast<std::string>(orders["build_unit"]);
    orderable->issue_order(order);
  } else if (order_name == "attack") {
    fw::debug << "issuing \"attack\" order to units." << std::endl;
    std::weak_ptr<ent::Entity> target_entity_wp;/// = luabind::object_cast<unit_wrapper*>(orders["target"])->get_entity();
    std::shared_ptr<ent::Entity> target_entity = target_entity_wp.lock();
    if (target_entity) {
      std::shared_ptr<AttackOrder> order = create_order<AttackOrder>();
      order->target = target_entity->get_id();
      orderable->issue_order(order);
    }
  } else {
    fw::debug << "unknown order!" << std::endl;
  }
}

fw::lua::Value AIPlayer::get_unit_wrapper(std::weak_ptr<ent::Entity> wp) {
  std::shared_ptr<ent::Entity> ent = wp.lock();
  if (!ent) {
    return fw::lua::Value();
  }

  ent::EntityAttribute *attr = ent->get_attribute("ai_wrapper");
  if (attr == nullptr) {
    fw::lua::Value wrapper = create_unit_wrapper(ent->get_name());
//    luabind::object_cast<unit_wrapper *>(wrapper)->set_entity(wp);
    ent->add_attribute(ent::EntityAttribute("ai_wrapper", wrapper));
    attr = ent->get_attribute("ai_wrapper");
  }

  return attr->get_value<fw::lua::Value>();
}

fw::lua::Value AIPlayer::create_unit_wrapper(std::string const &entity_name) {
  auto it = unit_creator_map_.find(entity_name);
//  if (it != _unit_creator_map.end() && it->second.is_valid()) {
//    return it->second() [luabind::adopt(luabind::result)];
//  }

  // if we don't have a specific wrapper for this unit type, just create a generic
  // one and return that instead
//  return luabind::call_function<luabind::object>(*_script, "Unit");
  return fw::lua::Value();
}

void AIPlayer::update() {
  update_queue_.update();
}

// this is called when our local player is ready to start the game
void AIPlayer::local_player_is_ready() {
}

void AIPlayer::world_loaded() {
  fw::Vector start_loc;

  auto start_it = game::World::get_instance()->get_player_starts().find(player_no_);
  if (start_it == game::World::get_instance()->get_player_starts().end()) {
    fw::debug << boost::format("WARN: no player_start defined for player %1%, choosing random")
        % static_cast<int>(player_no_) << std::endl;
    start_loc = fw::Vector(13.0f, 0, 13.0f); // <todo, Random
  } else {
    start_loc = start_it->second;
  }

  // todo: we should create a "capital" or "HQ" or something building instead of "factory"
  std::shared_ptr<CreateEntityCommand> cmd(create_command<CreateEntityCommand>());
  cmd->template_name = "factory";
  cmd->initial_position = start_loc;
  SimulationThread::get_instance()->post_command(cmd);

  fire_event("game_started");
}

// This is called each simulation frame when we get all the commands from other players
void AIPlayer::post_commands(std::vector<std::shared_ptr<Command>> &) {
}

}
