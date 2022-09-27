#pragma once

#include <boost/signals2.hpp>
#include <game/entities/entity.h>

namespace ent {

// This component is applied to any Entity which can take damage. Entities that "dish out" damage
// will query for this component and apply the damage.
class DamageableComponent: public EntityComponent, public boost::signals2::trackable {
private:
  std::string expl_name_;
  void check_explode(boost::any health_value);

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
};

}
