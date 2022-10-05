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

// This implementation of player provides an AI player so that you can play against the computer, if you don't
// have any friends.
class AIPlayer : public Player {
private:
  typedef std::map<std::string, std::vector<fw::lua::Value>> LuaEventMap;
  typedef std::map<std::string, fw::lua::Value> UnitCreatorMap;

  ScriptDesc script_desc_;
  std::shared_ptr<fw::lua::LuaContext> script_;
  UpdateQueue update_queue_;
  LuaEventMap event_map_;
  UnitCreatorMap unit_creator_map_;
  bool is_valid_;

  void fire_event(std::string const &event_name,
     std::map<std::string, std::string> const &parameters = std::map<std::string, std::string>());

  // Helper function that returns the unit_wrapper for the given Entity, or creates a new one if it doesn't already
  // exist.
  fw::lua::Userdata<UnitWrapper> get_unit_wrapper(std::weak_ptr<ent::Entity> wp);

  // Creates a unit_wrapper for the given entity.
  fw::lua::Userdata<UnitWrapper> create_unit_wrapper(std::shared_ptr<ent::Entity> ent);

  static void l_set_ready(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_say(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_local_say(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_timer(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_register_unit(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_event(fw::lua::MethodContext<AIPlayer>& ctx);
  static void l_find_units(fw::lua::MethodContext<AIPlayer>& ctx);
  fw::lua::Value find_units(fw::lua::Value filter);

  static void l_issue_order(fw::lua::MethodContext<AIPlayer>& ctx);
  void issue_order(UnitWrapper* unit, fw::lua::Value order);

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
  bool is_valid_state() { return is_valid_; }

  LUA_DECLARE_METATABLE(AIPlayer);
};

}
