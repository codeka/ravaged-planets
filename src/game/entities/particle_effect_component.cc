#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/particle_manager.h>
#include <framework/particle_effect.h>

#include <game/entities/entity_factory.h>
#include <game/entities/entity_manager.h>
#include <game/entities/particle_effect_component.h>
#include <game/entities/position_component.h>

namespace ent {

// Register the our component in this file with the entity_factory.
ENT_COMPONENT_REGISTER("particle-effect", particle_effect_component);

particle_effect_component::particle_effect_component() :
    _destroy_entity_on_complete(false), _our_position(nullptr) {
}

particle_effect_component::~particle_effect_component() {
  _effect->destroy();
}

void particle_effect_component::apply_template(std::shared_ptr<entity_component_template> comp_template) {
  BOOST_FOREACH(auto &kvp, comp_template->properties) {
    if (kvp.first == "EffectName") {
      _effect_name = kvp.second;
    } else if (kvp.first == "DestroyEntityOnComplete") {
      _destroy_entity_on_complete = boost::lexical_cast<bool>(kvp.second);
    }
  }

  entity_component::apply_template(comp_template);
}

void particle_effect_component::initialize() {
  fw::particle_manager *mgr = fw::framework::get_instance()->get_particle_mgr();
  _effect = mgr->create_effect(_effect_name);
  _our_position = std::shared_ptr<entity>(_entity)->get_component<position_component>();
}

void particle_effect_component::update(float) {
  if (_our_position != nullptr) {
    _effect->set_position(_our_position->get_position());
  }

  if (_destroy_entity_on_complete && _effect->is_dead()) {
    std::shared_ptr<entity>(_entity)->get_manager()->destroy(_entity);
  }
}

}
