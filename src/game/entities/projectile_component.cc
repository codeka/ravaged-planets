#include <memory>
#include <boost/lexical_cast.hpp>

#include <framework/logging.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/entity_manager.h>
#include <game/entities/projectile_component.h>
#include <game/entities/position_component.h>
#include <game/entities/moveable_component.h>
#include <game/entities/damageable_component.h>
#include <game/entities/selectable_component.h>
#include <game/world/world.h>
#include <game/world/terrain.h>

namespace ent {

// register the various components in this file with the entity_factory
ENT_COMPONENT_REGISTER("SeekingProjectile", SeekingProjectileComponent);
ENT_COMPONENT_REGISTER("BallisticProjectile", BallisticProjectileComponent);

//-------------------------------------------------------------------------
ProjectileComponent::ProjectileComponent() :
    our_moveable_(0), our_position_(nullptr), target_position_(nullptr) {
}

ProjectileComponent::~ProjectileComponent() {
}

void ProjectileComponent::initialize() {
  std::shared_ptr<ent::Entity> Entity(entity_);
  our_moveable_ = Entity->get_component<MoveableComponent>();
  our_position_ = Entity->get_component<PositionComponent>();

  // obviously, we're a projectile, so we don't want to avoid collisions!
  our_moveable_->set_avoid_collisions(false);
}

void ProjectileComponent::set_target(std::weak_ptr<Entity> target) {
  target_ = target;

  std::shared_ptr<Entity> sp = target.lock();
  if (sp)
    target_position_ = sp->get_component<PositionComponent>();
}

void ProjectileComponent::update(float) {
  bool exploded = false;
  std::shared_ptr<ent::Entity> Entity(entity_);

  // find the nearest damagable Entity - if it's closer than the "hit" distance, then we've hit them!
  std::shared_ptr<ent::Entity> nearest =
      our_position_->get_nearest_entity_with_component<DamageableComponent>().lock();
  std::shared_ptr<ent::Entity> creator = Entity->get_creator().lock();
  if (nearest && nearest != creator) {
    fw::Vector nearest_dir = our_position_->get_direction_to(nearest);
    float nearest_distance = nearest_dir.length();

    float hit_distance = 0.5f;
    SelectableComponent *selectable = nearest->get_component<SelectableComponent>();
    if (selectable != 0) {
      hit_distance = selectable->get_selection_radius();
    }

    if (nearest_distance < hit_distance) {
      explode(nearest);
      exploded = true;
    }
  }

  if (!exploded) {
    // check whether we've hit the ground
    game::Terrain *trn = game::World::get_instance()->get_terrain();

    fw::Vector pos = our_position_->get_position();
    float height = trn->get_height(pos[0], pos[2]);
    if (height > pos[1]) {
      explode (std::shared_ptr<ent::Entity>());
      exploded = true;
    }
  }
}

void ProjectileComponent::explode(std::shared_ptr<Entity> hit) {
  // if we hit someone, apply damage to them
  if (hit) {
    DamageableComponent *damageable = hit->get_component<DamageableComponent>();
    if (damageable != nullptr) {
      damageable->apply_damage(30.0f);
    }
  }

  // now, just set our health to zero and let our DamageableComponent handle it
  std::shared_ptr<ent::Entity> Entity(entity_);
  EntityAttribute *attr = Entity->get_attribute("health");
  attr->set_value(0.0f);
}

//-------------------------------------------------------------------------
SeekingProjectileComponent::SeekingProjectileComponent() :
    time_to_lock_(0) {
}

SeekingProjectileComponent::~SeekingProjectileComponent() {
}

void SeekingProjectileComponent::apply_template(fw::lua::Value tmpl) {
  time_to_lock_ = tmpl["TimeToLock"];
}

void SeekingProjectileComponent::update(float dt) {
  if (time_to_lock_ > 0.0f) {
    time_to_lock_ -= dt;

    // our time to lock hasn't expired yet, keep waiting...
    if (time_to_lock_ > 0.0f) {
      our_moveable_->set_intermediate_goal(our_position_->get_position() + our_position_->get_direction());
    } else {
      // we've locked, make it exactly 0 (it might be less)
      time_to_lock_ = 0.0f;
    }
  }

  // if the target is destroyed before we get there, we'll just keep
  // moving towards the "old" target location and explode there...
  std::shared_ptr<Entity> target = target_.lock();
  if (target && time_to_lock_ == 0) {
    // "seek" the target, just move towards it...
    our_moveable_->set_intermediate_goal(target_position_->get_position());
  }

  // the base class will check when it's time to explode
  ProjectileComponent::update(dt);
}

//-------------------------------------------------------------------------
BallisticProjectileComponent::BallisticProjectileComponent() :
    max_height_(10.0f) {
}

BallisticProjectileComponent::~BallisticProjectileComponent() {
}

void BallisticProjectileComponent::apply_template(fw::lua::Value tmpl) {
  max_height_ = tmpl["MaxHeight"];
}

void BallisticProjectileComponent::set_target(std::weak_ptr<Entity> target) {
  ProjectileComponent::set_target(target);
  std::shared_ptr<Entity> sptarget = target.lock();
  if (!sptarget)
    return;

  // when the target is set is when we do all of our calculations. Basically,
  // we've got three points: our initial position, the position of our target
  // and point half way between and _max_height metres above the initial position.
  // we use these three points to calculate a circle and set the initial direction
  // and turn radius on our moveable component so that we travel in a nice-looking
  // arc towards the goal.

  fw::Vector initial_pos = our_position_->get_position();
  fw::Vector goal_pos = target_position_->get_position();
  fw::Vector mid_pos = initial_pos + (goal_pos - initial_pos) + fw::Vector(0, max_height_, 0);

  // todo: this is *not* a nice-looking arc!
  our_position_->set_direction((mid_pos - initial_pos).normalized());
  our_moveable_->set_turn_speed(1.0f);
  our_moveable_->set_intermediate_goal(goal_pos);
}

}
