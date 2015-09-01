#pragma once

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace ent {

// This component is attached to entities that have weapons and can
// shoot other entities.
class weapon_component: public entity_component {
private:
  std::weak_ptr<entity> _target;
  std::string _fire_entity_name;
  fw::vector _fire_direction;
  float _time_to_fire;
  float _range;

  void fire();

public:
  static const int identifier = 500;

  weapon_component();
  virtual ~weapon_component();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void update(float dt);

  void set_target(std::weak_ptr<entity> target) {
    _target = target;
  }
  void clear_target() {
    _target.reset();
  }
  std::weak_ptr<entity> get_target() const {
    return _target;
  }

  virtual int get_identifier() {
    return identifier;
  }
};

}
