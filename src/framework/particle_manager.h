#pragma once

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include <framework/graphics.h>
#include <framework/object_pool.h>
#include <framework/status.h>
#include <framework/texture.h>

namespace fw {
class Particle;
class ParticleEmitter;
class ParticleEffect;
class ParticleRenderer;

/** The ParticleManager manages all of the Particle emitters in the game. */
class ParticleManager {
public:
  typedef std::list<std::shared_ptr<ParticleEffect>> EffectList;
  typedef std::list<std::weak_ptr<Particle>> ParticleList;

private:
  // Controls access to the effect list, which can be access from both the render thread and update
  // thread.
  std::mutex mutex_;

  ObjectPool<Particle> particle_pool_;
  Graphics *graphics_;
  ParticleRenderer *renderer_;
  EffectList effects_;
  ParticleList particles_;
  float wrap_x_;
  float wrap_z_;

  // If true, the game is paused, so we shouldn't update any particules either. Particulars are
  // updated on the render thread, so we need this to be atomic.
  std::atomic<bool> paused_;

public:
  ParticleManager();
  ~ParticleManager();

  fw::Status Initialize(Graphics *g);
  void update(float dt);

  // This is called by the ParticleRenderer when we're about to render the particles. We return the
  // particle list so that it can actually render the particles.
  ParticleList& on_render(float dt);

  // created the named effect (we load the properties from the given .wwpart file)
  fw::StatusOr<std::shared_ptr<ParticleEffect>> CreateEffect(
      std::string_view name, fw::Vector const &initial_position);

  // this is called by the ParticleEffect when it's finished.
  void remove_effect(const std::shared_ptr<ParticleEffect>& effect);

  // sets the wrapping of the world, once we get > (x, *, z) we wrap back to (0, *, 0).
  void set_world_wrap(float x, float z);

  float get_wrap_x() const {
    return wrap_x_;
  }

  float get_wrap_z() const {
    return wrap_z_;
  }

  /** Gets a count of the number of active (alive) particles, mostly for debugging. */
  long get_num_active_particles() const;

  // this is called by the ParticleEmitter to add a new Particle
  void add_particle(std::weak_ptr<Particle> p);
};

}
