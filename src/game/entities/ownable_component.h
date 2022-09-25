#pragma once

#include <game/entities/entity.h>

namespace game {
class player;
}

namespace ent {

class OwnableComponent: public EntityComponent {
private:
  game::player *owner_;

public:
  static const int identifier = 900;
  virtual int get_identifier() {
    return identifier;
  }

  OwnableComponent();
  ~OwnableComponent();

  game::player *get_owner() const {
    return owner_;
  }
  void set_owner(game::player *owner);

  // helper for checking whether this Entity is owned by the local player
  bool is_local_player() const;

  // helper for checking whether this Entity is owned by the local player or an ai player
  // that's under our control
  bool is_local_or_ai_player() const;

  // this signal is fired when the owner changes.
  boost::signals2::signal<void(OwnableComponent *)> owner_changed_event;
};

}
