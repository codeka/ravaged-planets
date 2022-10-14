
#include <framework/framework.h>
#include <framework/particle_manager.h>
#include <framework/particle_effect.h>

#include <game/entities/entity_factory.h>
#include <game/entities/entity_manager.h>
#include <game/entities/particle_effect_component.h>
#include <game/entities/position_component.h>

namespace ent {

// Register the our component in this file with the entity_factory.
ENT_COMPONENT_REGISTER("ParticleEffect", ParticleEffectComponent);

ParticleEffectComponent::ParticleEffectComponent() : our_position_(nullptr) {
}

ParticleEffectComponent::~ParticleEffectComponent() {
  for(auto& kvp : effects_) {
    EffectInfo const &effect_info = kvp.second;
    if (effect_info.effect) {
      effect_info.effect->destroy();
    }
  }
}

void ParticleEffectComponent::apply_template(fw::lua::Value tmpl) {
  for (auto& kvp : tmpl) {
    auto name = kvp.key<std::string>();
    auto value = kvp.value<fw::lua::Value>();
    EffectInfo effect;
    effect.name = value["EffectName"];
    effect.destroy_entity_on_complete = value["DestroyEntityOnComplete"];
    effect.started = value["Started"];
    if (value.has_key("Offset")) {
      std::string offset = value["Offset"];
      // TODO: parse offset
      effect.offset = fw::Vector(0, 2, 0);
    }

    effects_[name] = effect;
  }
}

void ParticleEffectComponent::initialize() {
  our_position_ = std::shared_ptr<Entity>(entity_)->get_component<PositionComponent>();
}

void ParticleEffectComponent::start_effect(std::string const &name) {
  auto it = effects_.find(name);
  if (it == effects_.end()) {
    return;
  }
  EffectInfo &effect_info = it->second;
  effect_info.started = true;
  fw::ParticleManager* mgr = fw::Framework::get_instance()->get_particle_mgr();
  effect_info.effect = mgr->create_effect(effect_info.name);
}

void ParticleEffectComponent::stop_effect(std::string const &name) {
  auto it = effects_.find(name);
  if (it == effects_.end()) {
    return;
  }
  EffectInfo &effect_info = it->second;
  effect_info.started = false;
  if (effect_info.effect) {
    effect_info.effect->destroy();
    effect_info.effect.reset();
  }
}

void ParticleEffectComponent::update(float) {
  fw::Vector our_position;
  if (our_position_) {
    our_position = our_position_->get_position();
  }

  for (auto& kvp : effects_) {
    EffectInfo& effect_info = kvp.second;
    if (!effect_info.effect) {
      return;
    }

    effect_info.effect->set_position(our_position + effect_info.offset);

    if (effect_info.destroy_entity_on_complete && effect_info.effect->is_dead()) {
      entity_.lock()->get_manager()->destroy(entity_);
    }
  }
}

}
