#pragma once

#include <map>
//#include <luabind/object.hpp>

#include <game/simulation/player.h>
#include <game/ai/update_queue.h>
#include <game/ai/ai_scriptmgr.h>
#include <game/ai/unit_wrapper.h>

namespace fw {
class lua_context;
}

namespace game {

/** This implementation of player uses Lua scripts to perform the actions of a player. */
class ai_player: public player {
private:
  typedef std::map<std::string, std::vector<std::shared_ptr<fw::lua_callback>>> lua_event_map;

  script_desc _script_desc;
  std::shared_ptr<fw::lua_context> _script;
  update_queue _upd_queue;
  lua_event_map _event_map;
  bool _is_valid;

  void fire_event(std::string const &event_name,
      std::map<std::string, std::string> const &parameters = std::map<std::string, std::string>());

  unit_wrapper *create_wrapper(std::string const &entity_name);
  void issue_order(unit_wrapper *unit, std::map<std::string, std::string> &orders);

  int l_set_ready(fw::lua_context &ctx);
  void set_ready();

  int l_say(fw::lua_context &ctx);
  void say(std::string const &msg);

  int l_local_say(fw::lua_context &ctx);
  void local_say(std::string const &msg);

  int l_timer(fw::lua_context &ctx);
  void timer(float dt, std::shared_ptr<fw::lua_callback> callback);

  int l_event(fw::lua_context &ctx);
  void event(std::string const &event_name, std::shared_ptr<fw::lua_callback> callback);

  int l_find_units(fw::lua_context &ctx);
  std::vector<unit_wrapper *> find_units(std::map<std::string, std::string> &params);

  int l_issue_order(fw::lua_context &ctx);
  void issue_order(std::vector<unit_wrapper *> &units, std::map<std::string, std::string> &orders);

public:
  static char const class_name[];
  static fw::lua_registrar<ai_player>::method_definition methods[];

  ai_player(std::string const &name, script_desc const &desc, uint8_t player_no);
  virtual ~ai_player();

  virtual void update();

  // this is called when our local player is ready to start the game
  virtual void local_player_is_ready();

  // this is called just after the world has loaded, we can create our initial entities and stuff
  virtual void world_loaded();

  // This is called each simulation frame when we get all the commands from
  // other players
  virtual void post_commands(std::vector<std::shared_ptr<command>> &commands);

  // gets value that indicates whether we're in a valid state or not
  bool is_valid_state() {
    return _is_valid;
  }
};

}
