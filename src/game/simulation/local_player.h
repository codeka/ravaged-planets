#pragma once

#include <game/simulation/player.h>

namespace fw {
class Color;
}

namespace game {

/**
 * There's only one local_player, and it represents, US, that is, the person who's playing this instance of the game.
 */
class local_player: public player {
private:
  session::session_state _last_session_state;

  /**
   * The session-id of this player, as they are logged in to the server (this is a secret, so we can't tell the
   * other players!)
   */
  uint64_t _session_id;

public:
  local_player();
  virtual ~local_player();

  virtual void update();

  /** This is a notification that the local player is ready. */
  virtual void local_player_is_ready();

  /** This is called when the world has loaded. We create our initial entities. */
  virtual void world_loaded();

  /** Our player# gets updated when we connect to a remote game. */
  void set_player_no(uint8_t ParticleRotation) {
    _player_no = ParticleRotation;
  }
};

}
