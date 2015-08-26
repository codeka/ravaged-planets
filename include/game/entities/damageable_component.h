#pragma once

#include <boost/signals2.hpp>
#include <game/entities/entity.h>

namespace ent {

/**
 * This component is applied to any entity which can take damage. Entities that "dish out" damage
 * will query for this component and apply the damage.
 */
class damageable_component: public entity_component,
    public boost::signals2::trackable {
private:
  std::string _expl_name;
  void check_explode(boost::any health_value);

public:
  static const int identifier = 450;
  virtual int get_identifier() {
    return identifier;
  }

  damageable_component();
  ~damageable_component();

  virtual void apply_template(luabind::object const &tmpl);
  virtual void initialize();

  void apply_damage(float amt);
  void explode();
};

}
