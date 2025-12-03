#include <functional>

#include <framework/framework.h>
#include <framework/graphics.h>

#include <game/entities/entity_factory.h>
#include <game/entities/entity_attribute.h>
#include <game/entities/entity_manager.h>
#include <game/entities/damageable_component.h>
#include <game/entities/position_component.h>

namespace ent {

using namespace std::placeholders;

// register the damageable component with the entity_factory
ENT_COMPONENT_REGISTER("Damageable", DamageableComponent);

DamageableComponent::DamageableComponent() {
}

DamageableComponent::~DamageableComponent() {
  std::shared_ptr<Entity> entity(entity_);
  EntityAttribute *health = entity->get_attribute("health");
  if (health != nullptr) {
    health->sig_value_changed.Disconnect(health_value_changed_signal_);
  }
}

void DamageableComponent::apply_template(fw::lua::Value tmpl) {
  expl_name_ = tmpl["Explosion"];
}

void DamageableComponent::initialize() {
  std::shared_ptr<Entity> entity(entity_);
  EntityAttribute *health = entity->get_attribute("health");
  if (health != nullptr) {
    health_value_changed_signal_ =
        health->sig_value_changed.Connect(std::bind(&DamageableComponent::check_explode, this, _2));
  }
}

void DamageableComponent::apply_damage(float amt) {
  std::shared_ptr<Entity> Entity(entity_);
  EntityAttribute *attr = Entity->get_attribute("health");
  if (attr != nullptr) {
    float curr_value = attr->get_value<float>();
    if (curr_value > 0) {
      attr->set_value(curr_value - amt);
    }
  }
}

// this is called whenever our health attribute changes value. we check whether it's
// hit 0, and explode if it has
void DamageableComponent::check_explode(std::any health_value) {
  if (std::any_cast<float>(health_value) <= 0) {
    explode();
  }
}

void apply_damage(std::shared_ptr<Entity> ent, float amt) {
  DamageableComponent *damageable = ent->get_component<DamageableComponent>();
  if (damageable != 0) {
    damageable->apply_damage(amt);
  }
}

void DamageableComponent::explode() {
  std::shared_ptr<Entity> entity(entity_); // Entity is always valid while we're valid...
  entity->get_manager()->destroy(entity_);

  if (expl_name_ != "") {
    // create an explosion entity
    entity->get_manager()->create_entity(entity, expl_name_, 0);
  }

  PositionComponent *our_position = entity->get_component<PositionComponent>();
  if (our_position != nullptr) {
    std::list<std::weak_ptr<ent::Entity>> entities;
    our_position->get_entities_within_radius(5.0f, std::back_inserter(entities));
    for(std::weak_ptr<ent::Entity> const &wp : entities) {
      std::shared_ptr<ent::Entity> ent = wp.lock();

      // don't damange ourselves...
      if (!ent || ent == entity)
        continue;

      ent::apply_damage(ent, (5.0f - our_position->get_direction_to(ent).length()));
    }
  }
}

}
