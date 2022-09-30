#pragma once

#include <map>

#include <framework/lua.h>

#include <game/simulation/player.h>
#include <game/ai/update_queue.h>
#include <game/ai/script_manager.h>
#include <game/ai/unit_wrapper.h>

namespace fw {
class LuaContext;
}

namespace game {

/**
 * This implementation of player provides an AI player so that you can play against the computer, if you don't
 * have any friends.
 */
class AIPlayer : public Player {
private:
  typedef std::map<std::string, std::vector<luabind::object>> lua_event_map;
  typedef std::map<std::string, luabind::object> unit_creator_map;

  ScriptDesc _script_desc;
  std::shared_ptr<fw::lua::LuaContext> _script;
  UpdateQueue _update_queue;
  lua_event_map _event_map;
  unit_creator_map _unit_creator_map;
  bool _is_valid;

  void fire_event(std::string const &event_name,
     std::map<std::string, std::string> const &parameters = std::map<std::string, std::string>());

  /** Helper function that returns the unit_wrapper (as a luabind::object) for the given Entity. */
  luabind::object get_unit_wrapper(std::weak_ptr<ent::Entity> wp);

  /** Creates a unit_wrapper for entities of the given type. */
  luabind::object create_unit_wrapper(std::string const &entity_name);

  void issue_order(UnitWrapper *unit, luabind::object orders);

  static void l_set_ready(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_say(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_local_say(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_timer(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_register_unit(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_event(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_find_units(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_issue_order(fw::lua::MethodContext<AIPlayer>& ctx);

public:
  AIPlayer(std::string const &name, ScriptDesc const &desc, uint8_t player_no);
  virtual ~AIPlayer();

  virtual void update();

  // this is called when our local player is ready to start the game
  virtual void local_player_is_ready();

  // this is called just after the world has loaded, we can create our initial entities and stuff
  virtual void world_loaded();

  // This is called each simulation frame when we get all the commands from
  // other players
  virtual void post_commands(std::vector<std::shared_ptr<Command> > &commands);

  // gets value that indicates whether we're in a valid state or not
  bool is_valid_state() { return _is_valid; }

  LUA_DECLARE_METATABLE(AIPlayer);
};

}
