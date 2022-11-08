#pragma once

#include <list>
#include <memory>

#include <framework/math.h>
#include <framework/object_pool.h>
#include <framework/xml.h>

namespace fw {
class ParticleManager;
class Particle;
class ParticleEmitterConfig;
class EmitPolicy;

/**
 * This class represents the Particle emitter, whose job it is to "emit" particles into the world with the correct
 * initial parameters and so on.
 */
class ParticleEmitter {
private:
  std::shared_ptr<ParticleEmitterConfig> config_;
  ObjectPool<Particle>& particle_pool_;
  ParticleManager *mgr_;
  float age_;
  int initial_count_;

  typedef std::list<std::shared_ptr<Particle>> ParticleList;
  ParticleList particles_;

  fw::Vector position_;
  EmitPolicy *emit_policy_;
  bool dead_;

public:
  ParticleEmitter(
    ParticleManager *mgr, ObjectPool<Particle>& particle_pool, std::shared_ptr<ParticleEmitterConfig> config,
    const fw::Vector& initial_position);
  ~ParticleEmitter();

  bool update(float dt);

  /**
   * Destroys the emitter. We'll stick around until all of our particles have died, but no new particles will be
   * created.
   */
  void destroy();

  void set_position(fw::Vector const &pos) {
    position_ = pos;
  }
  fw::Vector const &get_position() const {
    return position_;
  }

  ParticleManager *get_manager() const {
    return mgr_;
  }

  // This is called by the EmitPolicy when it decides to emit a new Particle.
  std::shared_ptr<Particle> emit(fw::Vector pos, float time_offset = 0.0f);
};

// This is the base class for the "policy" which decide how and when we emit new particles. It might be an "x per
// second" policy or a "when the new Particle will be x units from the last emitted Particle".
class EmitPolicy {
protected:
  ParticleEmitter* const emitter_;

public:
  EmitPolicy(ParticleEmitter* emitter) :
      emitter_(emitter) {
  }
  virtual ~EmitPolicy() {
  }

  virtual void check_emit(float) {
  }
};

// This is an EmitPolicy which emits particles at a certain rate every second.
class TimedEmitPolicy: public EmitPolicy {
private:
  float particles_per_second_;
  float time_since_last_particle_;
  fw::Vector last_position_;

public:
  TimedEmitPolicy(ParticleEmitter* emitter, float ParticleRotation);
  virtual ~TimedEmitPolicy();

  virtual void check_emit(float dt);
};

// This is an EmitPolicy that emits particles when the distance between the last one we emitted and the next one
// becomes greater than some threshold.
class DistanceEmitPolicy: public EmitPolicy {
private:
  std::weak_ptr<Particle> last_particle_;
  float max_distance_;

public:
  DistanceEmitPolicy(ParticleEmitter* emitter, float ParticleRotation);
  virtual ~DistanceEmitPolicy();

  virtual void check_emit(float dt);
};

// This is an EmitPolicy that doesn't ever actually emit anything. Presumably, you'd couple this with an "initial"
// count of particles to be emitted as soon as the emitter is started.
class NoEmitPolicy: public EmitPolicy {
public:
  NoEmitPolicy(ParticleEmitter* emitter, float ParticleRotation);
  virtual ~NoEmitPolicy();

  virtual void check_emit(float dt);
};

}
