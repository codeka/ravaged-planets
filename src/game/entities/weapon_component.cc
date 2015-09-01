#include <boost/foreach.hpp>

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
ENT_COMPONENT_REGISTER("Weapon", weapon_component);

weapon_component::weapon_component() :
    _time_to_fire(0.0f) {
}

weapon_component::~weapon_component() {
}

void weapon_component::apply_template(luabind::object const &tmpl) {
  _range = 10.0f;

  for (luabind::iterator it(tmpl), end; it != end; ++it) {
    if (it.key() == "FireEntity") {
      _fire_entity_name = luabind::object_cast<std::string>(*it);
    } else if (it.key() == "FireDirection") {
      // TODO: factor this out and use an actual array or something
      std::vector<float> parts = fw::split<float>(luabind::object_cast<std::string>(*it));
      if (parts.size() == 3) {
        _fire_direction = fw::vector(parts[0], parts[1], parts[2]);
      }
    } else if (it.key() == "Range") {
      _range = luabind::object_cast<float>(*it);
    }
  }
}

void weapon_component::update(float dt) {
  _time_to_fire -= dt;
  if (_time_to_fire < 0)
    _time_to_fire = 0.0f;

  std::shared_ptr<ent::entity> entity(_entity);
  std::shared_ptr<ent::entity> target = _target.lock();
  if (target) {
    position_component *our_pos = entity->get_component<position_component>();
    position_component *their_pos = target->get_component<position_component>();
    moveable_component *our_moveable = entity->get_component<moveable_component>();

    if (our_pos == nullptr || their_pos == nullptr)
      return;

    bool need_fire = true;
    if (our_moveable != nullptr) {
      float wrap_x = game::world::get_instance()->get_terrain()->get_width();
      float wrap_z = game::world::get_instance()->get_terrain()->get_length();
      fw::vector goal = fw::get_direction_to(our_pos->get_position(), their_pos->get_position(), wrap_x, wrap_z);
      if (goal.length() > _range) {
        our_moveable->set_goal(their_pos->get_position());
        need_fire = false;
      } else {
        our_moveable->stop();
      }
    }

    if (need_fire && _time_to_fire <= 0.0f) {
      fire();
      _time_to_fire = 5.0f;
    }
  }
}

void weapon_component::fire() {
  std::shared_ptr<ent::entity> entity(_entity);
  // TODO: entity_id
  std::shared_ptr<ent::entity> ent = entity->get_manager()->create_entity(entity, _fire_entity_name, 0);

  // it should face our "fire_direction"
  position_component *position = ent->get_component<position_component>();
  if (position != nullptr) {
    position->set_direction(_fire_direction);
  }

  // set the projectile's target to our target
  projectile_component *projectile = ent->get_component<projectile_component>();
  if (projectile != nullptr) {
    projectile->set_target(_target);
  }

  // fire our "Fire" audio cue
  //audio_component *our_audio = _entity->get_component<audio_component>();
  //if (our_audio != nullptr) {
  //  our_audio->play_cue("Fire");
  //}
}

}
