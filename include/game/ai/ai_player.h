#pragma once

#include <map>
#include <luabind/object.hpp>

#include <game/simulation/player.h>
#include <game/ai/update_queue.h>
#include <game/ai/ai_scriptmgr.h>
#include <game/ai/unit_wrapper.h>

namespace fw {
class lua_context;
}

namespace game {

/**
 * This implementation of player provides an AI player so that you can play against the computer, if you don't
 * have any friends.
 */
class ai_player : public player {
private:
  typedef std::map<std::string, std::vector<luabind::object>> lua_event_map;
  typedef std::map<std::string, luabind::object> unit_creator_map;

  script_desc _script_desc;
  std::shared_ptr<fw::lua_context> _script;
  update_queue _upd_queue;
  lua_event_map _event_map;
  unit_creator_map _unit_creator_map;
  bool _is_valid;

  void fire_event(std::string const &event_name,
     std::map<std::string, std::string> const &parameters = std::map<std::string, std::string>());

  luabind::object create_wrapper(std::string const &entity_name);
  void issue_order(unit_wrapper *unit, luabind::object orders);

  void l_set_ready();
  void l_say(std::string const &msg);
  void l_local_say(std::string const &msg);
  void l_timer(float dt, luabind::object obj);
  void l_register_unit(std::string name, luabind::object creator);
  void l_event(std::string const &event_name, luabind::object obj);
  luabind::object l_find_units(luabind::object params, lua_State* L);
  void l_issue_order(luabind::object units, luabind::object orders);

public:
  ai_player(std::string const &name, script_desc const &desc, uint8_t player_no);
  virtual ~ai_player();

  virtual void update();

  // this is called when our local player is ready to start the game
  virtual void local_player_is_ready();

  // this is called just after the world has loaded, we can create our initial entities and stuff
  virtual void world_loaded();

  // This is called each simulation frame when we get all the commands from
  // other players
  virtual void post_commands(std::vector<std::shared_ptr<command> > &commands);

  // gets value that indicates whether we're in a valid state or not
  bool is_valid_state() { return _is_valid; }
};

}
