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

particle_effect_component::particle_effect_component() : _our_position(nullptr) {
}

particle_effect_component::~particle_effect_component() {
  BOOST_FOREACH(auto kvp, effects_) {
    effect_info const &effect_info = kvp.second;
    if (effect_info.effect) {
      effect_info.effect->destroy();
    }
  }
}

void particle_effect_component::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    std::string name = luabind::object_cast<std::string>(it.key());
//    effect_info effect;
//    effect.name = luabind::object_cast<std::string>((*it)["EffectName"]);
//    if ((*it)["DestroyEntityOnComplete"]) {
//      effect.destroy_entity_on_complete = true;
//    }
//    if ((*it)["Started"]) {
//      effect.started = true;
//    }
//    if ((*it)["Offset"]) {
//      std::string offset = luabind::object_cast<std::string>((*it)["Offset"]);
//      effect.offset = fw::Vector(0, 2, 0); // TODO
//    }
//    effects_[name] = effect;
//  }
}

void particle_effect_component::initialize() {
  _our_position = std::shared_ptr<entity>(_entity)->get_component<position_component>();
}

void particle_effect_component::start_effect(std::string const &name) {
  auto it = effects_.find(name);
  if (it == effects_.end()) {
    return;
  }
  effect_info &effect_info = it->second;
  effect_info.started = true;
}

void particle_effect_component::stop_effect(std::string const &name) {
  auto it = effects_.find(name);
  if (it == effects_.end()) {
    return;
  }
  effect_info &effect_info = it->second;
  effect_info.started = false;
  if (effect_info.effect) {
    effect_info.effect->destroy();
  }
  effect_info.effect = std::shared_ptr<fw::ParticleEffect>();
}

void particle_effect_component::update(float) {
  BOOST_FOREACH(auto kvp, effects_) {
    effect_info const &effect_info = kvp.second;
    if (!effect_info.effect) {
      continue;
    }

    if (_our_position != nullptr) {
      effect_info.effect->set_position(_our_position->get_position() + effect_info.offset);
    }

    if (effect_info.destroy_entity_on_complete && effect_info.effect->is_dead()) {
      std::shared_ptr<entity>(_entity)->get_manager()->destroy(_entity);
    }
  }
}

void particle_effect_component::render(fw::sg::scenegraph &, fw::Matrix const &) {
  fw::ParticleManager *mgr = fw::framework::get_instance()->get_particle_mgr();
  BOOST_FOREACH(auto &kvp, effects_) {
    effect_info &effect_info = kvp.second;
    if (effect_info.started && !effect_info.effect) {
      effect_info.effect = mgr->create_effect(effect_info.name);
    }
  }
}

}
