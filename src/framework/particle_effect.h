#pragma once

#include <memory>
#include <vector>

#include <framework/particle_emitter.h>
#include <framework/vector.h>

namespace fw {
class ParticleManager;
class ParticleEffectConfig;

// A Particle effect represents, basically, a collection of particle_emitters and is what you get when you "load"
// an effect.
class ParticleEffect {
public:
  typedef std::vector<std::shared_ptr<ParticleEmitter>> EmitterList;

private:
  std::shared_ptr<ParticleEffectConfig> config_;
  ParticleManager *mgr_;
  EmitterList emitters_;
  bool dead_;

public:
  ParticleEffect(ParticleManager *mgr, std::shared_ptr<ParticleEffectConfig> const &config);
  ~ParticleEffect();

  void initialize();
  void destroy();

  void set_position(fw::Vector const &pos);
  void update(float dt);

  bool is_dead() const {
    return dead_;
  }

  // Returns true if we're both dead and all our emitters are also dead.
  bool is_finished() const {
    return dead_ && emitters_.size() == 0;
  }
};

}
