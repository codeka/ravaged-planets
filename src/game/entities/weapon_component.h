#pragma once

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace ent {

// This component is attached to entities that have weapons and can shoot other entities.
class WeaponComponent: public EntityComponent {
private:
  std::weak_ptr<Entity> target_;
  std::string fire_entity_name_;
  fw::Vector fire_direction_;
  float time_to_fire_;
  float range_;

  void fire();

public:
  static const int identifier = 500;

  WeaponComponent();
  virtual ~WeaponComponent();

  void apply_template(fw::lua::Value tmpl) override;

  virtual void update(float dt);

  void set_target(std::weak_ptr<Entity> target) {
    target_ = target;
  }
  void clear_target() {
    target_.reset();
  }
  std::weak_ptr<Entity> get_target() const {
    return target_;
  }

  virtual int get_identifier() {
    return identifier;
  }
};

}
