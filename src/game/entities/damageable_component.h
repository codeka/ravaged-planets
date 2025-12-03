#pragma once

#include <any>

#include <framework/signals.h>

#include <game/entities/entity.h>

namespace ent {

// This component is applied to any Entity which can take damage. Entities that "dish out" damage
// will query for this component and apply the damage.
class DamageableComponent: public EntityComponent {

public:
  static const int identifier = 450;
  virtual int get_identifier() {
    return identifier;
  }

  DamageableComponent();
  ~DamageableComponent();

  void apply_template(fw::lua::Value tmpl) override;
  virtual void initialize();

  void apply_damage(float amt);
  void explode();

private:
  void check_explode(std::any health_value);
  
  fw::SignalConnection health_value_changed_signal_;
  std::string expl_name_;
};

}
