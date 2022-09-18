#pragma once

#include <game/entities/entity.h>

namespace game {
class player;
}

namespace ent {

/**
 * This component is used to make an entity visible on the minimap. The entity must also have a position_component and
 * optionally an ownable_component (to get a colour - if no ownable colour, it gets drawn white.)
 */
class minimap_visible_component: public entity_component {
public:
  static const int identifier = 150;
  virtual int get_identifier() {
    return identifier;
  }

  virtual bool allow_get_by_component() {
    return true;
  }

  minimap_visible_component();
  ~minimap_visible_component();
};

}
