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
ENT_COMPONENT_REGISTER("ParticleEffect", particle_effect_component);

particle_effect_component::particle_effect_component() :
    _destroy_entity_on_complete(false), _our_position(nullptr) {
}

particle_effect_component::~particle_effect_component() {
  _effect->destroy();
}

void particle_effect_component::apply_template(luabind::object const &tmpl) {
  for (luabind::iterator it(tmpl), end; it != end; ++it) {
    if (it.key() == "EffectName") {
      _effect_name = luabind::object_cast<std::string>(*it);
    } else if (it.key() == "DestroyEntityOnComplete") {
      _destroy_entity_on_complete = luabind::object_cast<bool>(*it);
    }
  }
}

void particle_effect_component::initialize() {
  _our_position = std::shared_ptr<entity>(_entity)->get_component<position_component>();
}

void particle_effect_component::update(float) {
  if (!_effect) {
    // It might not be created yet (since it only gets created on the render thread when we render).
    return;
  }

  if (_our_position != nullptr) {
    _effect->set_position(_our_position->get_position());
  }

  if (_destroy_entity_on_complete && _effect->is_dead()) {
    std::shared_ptr<entity>(_entity)->get_manager()->destroy(_entity);
  }
}

void particle_effect_component::render(fw::sg::scenegraph &, fw::matrix const &) {
  if (!_effect) {
    fw::particle_manager *mgr = fw::framework::get_instance()->get_particle_mgr();
    _effect = mgr->create_effect(_effect_name);
  }
}


}
