#include <boost/foreach.hpp>

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
ENT_COMPONENT_REGISTER("weapon", weapon_component);

weapon_component::weapon_component() :
    _time_to_fire(0.0f) {
}

weapon_component::~weapon_component() {
}

void weapon_component::apply_template(std::shared_ptr<entity_component_template> comp_template) {
  BOOST_FOREACH(auto &kvp, comp_template->properties) {
    if (kvp.first == "FireEntity") {
      _fire_entity_name = kvp.second;
    } else if (kvp.first == "FireDirection") {
      std::vector<float> parts = fw::split<float>(kvp.second);
      if (parts.size() == 3) {
        _fire_direction = fw::vector(parts[0], parts[1], parts[2]);
      }
    }
  }

  return entity_component::apply_template(comp_template);
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

    if (our_moveable != nullptr) {
      float wrap_x = game::world::get_instance()->get_terrain()->get_width();
      float wrap_z = game::world::get_instance()->get_terrain()->get_length();
      fw::vector goal = fw::get_direction_to(our_pos->get_position(), their_pos->get_position(), wrap_x, wrap_z);
      if (goal.length() > 100.0f) {
        our_moveable->set_goal(their_pos->get_position());
      } else {
        our_moveable->set_goal(our_pos->get_position());

        if (_time_to_fire <= 0.0f) {
          fire();
          _time_to_fire = 5.0f;
        }
      }
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
  //audio_component *our_audio = entity->get_component<audio_component>();
  //if (our_audio != nullptr) {
  //  our_audio->play_cue("Fire");
  //}
}

}
