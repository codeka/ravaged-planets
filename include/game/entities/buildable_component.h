#pragma once

#include <game/entities/entity.h>

namespace ent {

/**
 * This component is attached to entities that can be directly built by a builder_component entity.
 */
class buildable_component: public entity_component {
private:
  std::string _build_group;

public:
  static const int identifier = 350;
  virtual int get_identifier() {
    return identifier;
  }

  buildable_component();
  ~buildable_component();

  virtual void apply_template(std::shared_ptr<entity_component_template> comp_template);
};

}
