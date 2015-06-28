#pragma once

#include <framework/colour.h>
#include <game/session/session.h>
#include <game/simulation/commands.h>

namespace game {
class command;

extern std::vector<fw::colour> player_colours;

/**
 * This is the base class for the players. There's three kinds of players in the game, the local player (only one of
 * these), remote players (which are connected to us via the internet) and AI players.
 */
class player {
protected:
  bool _is_ready_to_start;
  uint8_t _player_no;
  uint32_t _user_id;
  std::string _user_name;
  fw::colour _colour;

  void player_ready();

  /** This is just a helper so we don't have to worry about passing the _player_no when creating new commands. */
  template<typename T>
  inline std::shared_ptr<T> create_command() {
    return ::game::create_command<T>(_player_no);
  }

  player();
public:
  virtual ~player();

  virtual void update();
  virtual void send_chat_msg(std::string const &msg);

  /** This is called when our local player is ready to start the game, we should notify our peer that we're ready. */
  virtual void local_player_is_ready() = 0;

  /** Gets a value which indicates whether this player is ready to start the game. */
  bool is_ready() const {
    return _is_ready_to_start;
  }

  /** This is called just after the world has loaded, we can create our initial entities and stuff. */
  virtual void world_loaded();

  /** Posts the given commands to this player for the next turn. */
  virtual void post_commands(std::vector<std::shared_ptr<command>> &commands);

  std::string get_user_name() const;
  uint32_t get_user_id() const {
    return _user_id == 0 ? _player_no : _user_id;
  }
  uint8_t get_player_no() const {
    return _player_no;
  }
  fw::colour get_colour() const {
    return _colour;
  }

  /** Sets our colour to the given value. */
  void set_colour(fw::colour const &col) {
    _colour = col;
  }
};

}
