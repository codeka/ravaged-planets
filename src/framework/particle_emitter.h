#pragma once

#include <list>
#include <memory>

#include <framework/vector.h>
#include <framework/xml.h>

namespace fw {
class particle_manager;
class particle;
class particle_emitter_config;
class emit_policy;

/**
 * This class represents the particle emitter, whose job it is to "emit" particles into the world with the correct
 * initial parameters and so on.
 */
class particle_emitter {
private:
  std::shared_ptr<particle_emitter_config> _config;
  particle_manager *_mgr;
  float _age;
  int _initial_count;

  typedef std::list<particle *> particle_list;
  particle_list _particles;

  fw::vector _position;
  emit_policy *_emit_policy;
  bool _dead;

public:
  particle_emitter(particle_manager *mgr, std::shared_ptr<particle_emitter_config> config);
  ~particle_emitter();

  void initialize();
  bool update(float dt);

  /**
   * Destroys the emitter. We'll stick around until all of our particles have died, but no new particles will be
   * created.
   */
  void destroy();

  void set_position(fw::vector const &pos) {
    _position = pos;
  }
  fw::vector const &get_position() const {
    return _position;
  }

  particle_manager *get_manager() const {
    return _mgr;
  }

  /** This is called by the emit_policy when it decides to emit a new particle. */
  particle *emit(fw::vector pos, float time_offset = 0.0f);
};

/**
 * This is the base class for the "policy" which decide how and when we emit new particles. It might be an "x per
 * second" policy or a "when the new particle will be x units from the last emitted particle".
 */
class emit_policy {
protected:
  particle_emitter *_emitter;

public:
  emit_policy() :
      _emitter(0) {
  }
  virtual ~emit_policy() {
  }

  virtual void initialize(particle_emitter *emitter);

  virtual void check_emit(float) {
  }
};

/** This is an emit_policy which emits particles at a certain rate every second. */
class timed_emit_policy: public emit_policy {
private:
  float _particles_per_second;
  float _time_since_last_particle;
  fw::vector _last_position;

public:
  timed_emit_policy(float value);
  virtual ~timed_emit_policy();

  virtual void check_emit(float dt);
};

/**
 * This is an emit_policy that emits particles when the distance between the last one we emitted and the next one
 * becomes greater than some threshold.
 */
class distance_emit_policy: public emit_policy {
private:
  particle *_last_particle;
  float _max_distance;

public:
  distance_emit_policy(float value);
  virtual ~distance_emit_policy();

  virtual void check_emit(float dt);
};

/**
 * This is an emit_policy that doesn't ever actually emit anything. Presumably, you'd couple this with an "initial"
 * count of particles to be emitted as soon as the emitter is started.
 */
class no_emit_policy: public emit_policy {
public:
  no_emit_policy(float value);
  virtual ~no_emit_policy();

  virtual void check_emit(float dt);
};

}
