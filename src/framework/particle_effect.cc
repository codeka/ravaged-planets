
#include <framework/math.h>
#include <framework/particle_effect.h>
#include <framework/particle_manager.h>
#include <framework/particle_config.h>

namespace fw {

ParticleEffect::ParticleEffect(
  ParticleManager *mgr, ObjectPool<Particle>& particle_pool, std::shared_ptr<ParticleEffectConfig> const &config, const fw::Vector& initial_position) :
    mgr_(mgr), dead_(false) {

  for (auto it = config->emitter_config_begin(); it != config->emitter_config_end(); ++it) {
    std::shared_ptr<ParticleEmitter> emitter(new ParticleEmitter(mgr_, particle_pool, *it, initial_position));
    emitters_.push_back(emitter);
  }
}

ParticleEffect::~ParticleEffect() {
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
}

}
