
#include <framework/particle_effect.h>
#include <framework/particle_manager.h>
#include <framework/particle_config.h>
#include <framework/vector.h>

namespace fw {

ParticleEffect::ParticleEffect(ParticleManager *mgr, std::shared_ptr<ParticleEffectConfig> const &config) :
    mgr_(mgr), config_(config), dead_(false) {
}

ParticleEffect::~ParticleEffect() {
}

void ParticleEffect::initialize() {
  for (auto it = config_->emitter_config_begin(); it != config_->emitter_config_end(); ++it) {
    std::shared_ptr<ParticleEmitter> emitter(new ParticleEmitter(mgr_, *it));
    emitter->initialize();
    emitters_.push_back(emitter);
  }
}

void ParticleEffect::destroy() {
  // destroy any emitters that are still running
  for(auto &emitter : emitters_) {
    emitter->destroy();
  }

  // and set our own "dead" flag. when all emitters have finished, we'll destroy ourselves.
  dead_ = true;
}

void ParticleEffect::set_position(fw::Vector const &pos) {
  for(auto& emitter : emitters_) {
    emitter->set_position(pos);
  }
}

void ParticleEffect::update(float dt) {
  std::vector<ParticleEmitter *> to_delete;

  for(auto& emitter : emitters_) {
    if (!emitter->update(dt)) {
      to_delete.push_back(emitter.get());
    }
  }

  for (auto dit = to_delete.begin(); dit != to_delete.end(); ++dit) {
    for (auto it = emitters_.begin(); it != emitters_.end(); ++it) {
      if ((*it).get() == *dit) {
        emitters_.erase(it);
        break;
      }
    }
  }

  if (dead_ && emitters_.size() == 0) {
    mgr_->remove_effect(this);
  }
}

}
