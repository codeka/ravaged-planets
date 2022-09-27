#pragma once

#include <game/entities/entity.h>

namespace ent {

/**
 * This component is attached to entities that can be directly built by a BuilderComponent Entity.
 */
class BuildableComponent: public EntityComponent {
private:
  std::string build_group_;

public:
  static const int identifier = 350;
  virtual int get_identifier() {
    return identifier;
  }

  BuildableComponent();
  ~BuildableComponent();

  void apply_template(fw::lua::Value tmpl) override;
};

}
