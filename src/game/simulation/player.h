#pragma once

#include <framework/color.h>
#include <game/session/session.h>
#include <game/simulation/commands.h>

namespace game {
class Command;

extern std::vector<fw::Color> player_colors;

/**
 * This is the base class for the players. There's three kinds of players in the game, the local player (only one of
 * these), remote players (which are connected to us via the internet) and AI players.
 */
class Player {
protected:
  bool is_ready_to_start_;
  uint8_t player_no_;
  uint32_t user_id_;
  std::string user_name_;
  fw::Color color_;

  void player_ready();

  /** This is just a helper so we don't have to worry about passing the _player_no when creating new commands. */
  template<typename T>
  inline std::shared_ptr<T> create_command() {
    return ::game::create_command<T>(player_no_);
  }

  Player();
public:
  virtual ~Player();

  virtual void update();
  virtual void send_chat_msg(std::string const &msg);

  /** This is called when our local player is ready to start the game, we should notify our Peer that we're ready. */
  virtual void local_player_is_ready() = 0;

  /** Gets a value which indicates whether this player is ready to start the game. */
  bool is_ready() const {
    return is_ready_to_start_;
  }

  /** This is called just after the world has loaded, we can create our initial entities and stuff. */
  virtual void world_loaded();

  /** Posts the given commands to this player for the next turn. */
  virtual void post_commands(std::vector<std::shared_ptr<Command>> &commands);

  std::string get_user_name() const;
  uint32_t get_user_id() const {
    return user_id_ == 0 ? player_no_ : user_id_;
  }
  uint8_t get_player_no() const {
    return player_no_;
  }
  fw::Color get_color() const {
    return color_;
  }

  /** Sets our color to the given value. */
  void set_color(fw::Color const &col) {
    color_ = col;
  }
};

}
