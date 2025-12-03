#pragma once

#include <framework/signals.h>

#include <game/entities/entity.h>

namespace game {
class Player;
}

namespace ent {

class OwnableComponent: public EntityComponent {
private:
  std::shared_ptr<game::Player> owner_;

public:
  static const int identifier = 900;
  virtual int get_identifier() {
    return identifier;
  }

  OwnableComponent();
  ~OwnableComponent();

  std::shared_ptr<game::Player> get_owner() const {
    return owner_;
  }
  void set_owner(std::shared_ptr<game::Player> const &owner);

  // helper for checking whether this Entity is owned by the local player
  bool is_local_player() const;

  // helper for checking whether this Entity is owned by the local player or an ai player
  // that's under our control
  bool is_local_or_ai_player() const;

  // this signal is fired when the owner changes.
  fw::Signal<OwnableComponent *> owner_changed_event;
};

}
