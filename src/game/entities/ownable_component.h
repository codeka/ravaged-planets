#pragma once

#include <game/entities/entity.h>

namespace game {
class player;
}

namespace ent {

class ownable_component: public entity_component {
private:
  game::player *_owner;

public:
  static const int identifier = 900;
  virtual int get_identifier() {
    return identifier;
  }

  ownable_component();
  ~ownable_component();

  game::player *get_owner() const {
    return _owner;
  }
  void set_owner(game::player *owner);

  // helper for checking whether this entity is owned by the local player
  bool is_local_player() const;

  // helper for checking whether this entity is owned by the local player or an ai player
  // that's under our control
  bool is_local_or_ai_player() const;

  // this signal is fired when the owner changes.
  boost::signals2::signal<void(ownable_component *)> owner_changed_event;
};

}
