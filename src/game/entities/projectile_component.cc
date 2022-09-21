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
ENT_COMPONENT_REGISTER("SeekingProjectile", seeking_projectile_component);
ENT_COMPONENT_REGISTER("BallisticProjectile", ballistic_projectile_component);

//-------------------------------------------------------------------------
projectile_component::projectile_component() :
    _our_moveable(0), _our_position(nullptr), _target_position(nullptr) {
}

projectile_component::~projectile_component() {
}

void projectile_component::initialize() {
  std::shared_ptr<ent::entity> entity(_entity);
  _our_moveable = entity->get_component<moveable_component>();
  _our_position = entity->get_component<position_component>();

  // obviously, we're a projectile, so we don't want to avoid collisions!
  _our_moveable->set_avoid_collisions(false);
}

void projectile_component::set_target(std::weak_ptr<entity> target) {
  _target = target;

  std::shared_ptr<entity> sp = target.lock();
  if (sp)
    _target_position = sp->get_component<position_component>();
}

void projectile_component::update(float) {
  bool exploded = false;
  std::shared_ptr<ent::entity> entity(_entity);

  // find the nearest damagable entity - if it's closer than the "hit" distance, then we've hit them!
  std::shared_ptr<ent::entity> nearest =
      _our_position->get_nearest_entity_with_component<damageable_component>().lock();
  std::shared_ptr<ent::entity> creator = entity->get_creator().lock();
  if (nearest && nearest != creator) {
    fw::Vector nearest_dir = _our_position->get_direction_to(nearest);
    float nearest_distance = nearest_dir.length();

    float hit_distance = 0.5f;
    selectable_component *selectable = nearest->get_component<selectable_component>();
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
    game::terrain *trn = game::world::get_instance()->get_terrain();

    fw::Vector pos = _our_position->get_position();
    float height = trn->get_height(pos[0], pos[2]);
    if (height > pos[1]) {
      explode (std::shared_ptr<ent::entity>());
      exploded = true;
    }
  }
}

void projectile_component::explode(std::shared_ptr<entity> hit) {
  // if we hit someone, apply damage to them
  if (hit) {
    damageable_component *damageable = hit->get_component<damageable_component>();
    if (damageable != nullptr) {
      damageable->apply_damage(30.0f);
    }
  }

  // now, just set our health to zero and let our damageable_component handle it
  std::shared_ptr<ent::entity> entity(_entity);
  entity_attribute *attr = entity->get_attribute("health");
  attr->set_value(0.0f);
}

//-------------------------------------------------------------------------
seeking_projectile_component::seeking_projectile_component() :
    _time_to_lock(0) {
}

seeking_projectile_component::~seeking_projectile_component() {
}

void seeking_projectile_component::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "TimeToLock") {
//      _time_to_lock = luabind::object_cast<float>(*it);
//    }
//  }
}

void seeking_projectile_component::update(float dt) {
  if (_time_to_lock > 0.0f) {
    _time_to_lock -= dt;

    // our time to lock hasn't expired yet, keep waiting...
    if (_time_to_lock > 0.0f) {
      _our_moveable->set_intermediate_goal(_our_position->get_position() + _our_position->get_direction());
    } else {
      // we've locked, make it exactly 0 (it might be less)
      _time_to_lock = 0.0f;
    }
  }

  // if the target is destroyed before we get there, we'll just keep
  // moving towards the "old" target location and explode there...
  std::shared_ptr<entity> target = _target.lock();
  if (target && _time_to_lock == 0) {
    // "seek" the target, just move towards it...
    _our_moveable->set_intermediate_goal(_target_position->get_position());
  }

  // the base class will check when it's time to explode
  projectile_component::update(dt);
}

//-------------------------------------------------------------------------
ballistic_projectile_component::ballistic_projectile_component() :
    _max_height(10.0f) {
}

ballistic_projectile_component::~ballistic_projectile_component() {
}

void ballistic_projectile_component::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "MaxHeight") {
//      _max_height = luabind::object_cast<float>(*it);
//    }
//  }
}

void ballistic_projectile_component::set_target(std::weak_ptr<entity> target) {
  projectile_component::set_target(target);
  std::shared_ptr<entity> sptarget = target.lock();
  if (!sptarget)
    return;

  // when the target is set is when we do all of our calculations. Basically,
  // we've got three points: our initial position, the position of our target
  // and point half way between and _max_height metres above the initial position.
  // we use these three points to calculate a circle and set the initial direction
  // and turn radius on our moveable component so that we travel in a nice-looking
  // arc towards the goal.

  fw::Vector initial_pos = _our_position->get_position();
  fw::Vector goal_pos = _target_position->get_position();
  fw::Vector mid_pos = initial_pos + (goal_pos - initial_pos) + fw::Vector(0, _max_height, 0);

  // todo: this is *not* a nice-looking arc!
  _our_position->set_direction((mid_pos - initial_pos).normalize());
  _our_moveable->set_turn_speed(1.0f);
  _our_moveable->set_intermediate_goal(goal_pos);
}

}
