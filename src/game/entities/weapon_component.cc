
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/misc.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/entity_manager.h>
#include <game/entities/weapon_component.h>
#include <game/entities/position_component.h>
#include <game/entities/moveable_component.h>
#include <game/entities/projectile_component.h>
#include <game/entities/audio_component.h>
#include <game/world/world.h>
#include <game/world/terrain.h>

namespace ent {

// register the component with the entity_factory
ENT_COMPONENT_REGISTER("Weapon", WeaponComponent);

WeaponComponent::WeaponComponent() :
    time_to_fire_(0.0f) {
}

WeaponComponent::~WeaponComponent() {
}

void WeaponComponent::apply_template(luabind::object const &tmpl) {
  range_ = 10.0f;

//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "FireEntity") {
//      _fire_entity_name = luabind::object_cast<std::string>(*it);
//    } else if (it.key() == "FireDirection") {
//      // TODO: factor this out and use an actual array or something
//      std::vector<float> parts = fw::split<float>(luabind::object_cast<std::string>(*it));
//      if (parts.size() == 3) {
//        _fire_direction = fw::Vector(parts[0], parts[1], parts[2]);
//      }
//    } else if (it.key() == "Range") {
//      _range = luabind::object_cast<float>(*it);
//    }
//  }
}

void WeaponComponent::update(float dt) {
  time_to_fire_ -= dt;
  if (time_to_fire_ < 0)
    time_to_fire_ = 0.0f;

  std::shared_ptr<ent::Entity> Entity(entity_);
  std::shared_ptr<ent::Entity> target = target_.lock();
  if (target) {
    PositionComponent *our_pos = Entity->get_component<PositionComponent>();
    PositionComponent *their_pos = target->get_component<PositionComponent>();
    MoveableComponent *our_moveable = Entity->get_component<MoveableComponent>();

    if (our_pos == nullptr || their_pos == nullptr)
      return;

    bool need_fire = true;
    if (our_moveable != nullptr) {
      float wrap_x = game::World::get_instance()->get_terrain()->get_width();
      float wrap_z = game::World::get_instance()->get_terrain()->get_length();
      fw::Vector goal = fw::get_direction_to(our_pos->get_position(), their_pos->get_position(), wrap_x, wrap_z);
      if (goal.length() > range_) {
        our_moveable->set_goal(their_pos->get_position());
        need_fire = false;
      } else {
        our_moveable->stop();
      }
    }

    if (need_fire && time_to_fire_ <= 0.0f) {
      fire();
      time_to_fire_ = 5.0f;
    }
  }
}

void WeaponComponent::fire() {
  std::shared_ptr<ent::Entity> Entity(entity_);
  // TODO: entity_id
  std::shared_ptr<ent::Entity> ent = Entity->get_manager()->create_entity(Entity, fire_entity_name_, 0);

  // it should face our "fire_direction"
  PositionComponent *position = ent->get_component<PositionComponent>();
  if (position != nullptr) {
    position->set_direction(fire_direction_);
  }

  // set the projectile's target to our target
  ProjectileComponent *projectile = ent->get_component<ProjectileComponent>();
  if (projectile != nullptr) {
    projectile->set_target(target_);
  }

  // fire our "Fire" audio Cue
  //AudioComponent *our_audio = entity_->get_component<AudioComponent>();
  //if (our_audio != nullptr) {
  //  our_audio->play_cue("Fire");
  //}
}

}
