#pragma once

#include <memory>
#include <vector>

#include <framework/particle_emitter.h>
#include <framework/vector.h>

namespace fw {
class particle_manager;
class particle_effect_config;

/**
 * A particle effect represents, basically, a collection of particle_emitters and is what you get when you "load"
 * an effect.
 */
class particle_effect {
public:
  typedef std::vector<std::shared_ptr<particle_emitter>> emitter_list;

private:
  std::shared_ptr<particle_effect_config> _config;
  particle_manager *_mgr;
  emitter_list _emitters;
  bool _dead;

public:
  particle_effect(particle_manager *mgr, std::shared_ptr<particle_effect_config> const &config);
  ~particle_effect();

  void initialize();
  void destroy();

  void set_position(fw::vector const &pos);
  void update(float dt);

  bool is_dead() const {
    return _dead;
  }
};

}
