#pragma once

#include <game/simulation/player.h>

namespace fw {
class Color;
}

namespace game {

// There's only one local_player, and it represents, US, that is, the person who's playing this instance of the game.
class LocalPlayer: public Player {
private:
  session::session_state last_session_state_;

  // The session-id of this player, as they are logged in to the server (this is a secret, so we can't tell the
  // other players!)
  uint64_t session_id_;

public:
  LocalPlayer();
  virtual ~LocalPlayer();

  void update() override;

  /** This is a notification that the local player is ready. */
  void local_player_is_ready() override;

  /** This is called when the world has loaded. We create our initial entities. */
  void world_loaded() override;

  /** Our player# gets updated when we connect to a remote game. */
  void set_player_no(uint8_t value) {
    player_no_ = value;
  }
};

}
