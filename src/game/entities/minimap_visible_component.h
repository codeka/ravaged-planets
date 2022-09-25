#pragma once

#include <game/entities/entity.h>

namespace game {
class Player;
}

namespace ent {

// This component is used to make an Entity visible on the minimap. The Entity must also have a position_component and
// optionally an ownable_component (to get a color - if no ownable color, it gets drawn white.)
class MinimapVisibleComponent: public EntityComponent {
public:
  static const int identifier = 150;
  virtual int get_identifier() {
    return identifier;
  }

  virtual bool allow_get_by_component() {
    return true;
  }

  MinimapVisibleComponent();
  ~MinimapVisibleComponent();
};

}
