#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include <framework/graphics.h>
#include <framework/scenegraph.h>
#include <framework/texture.h>

namespace fw {
class particle;
class particle_emitter;
class particle_effect;
class particle_renderer;

/** The particle_manager manages all of the particle emitters in the game. */
class particle_manager {
public:
  typedef std::list<std::shared_ptr<particle_effect> > effect_list;
  typedef std::list<particle *> particle_list;

private:
  Graphics *_graphics;
  particle_renderer *_renderer;
  effect_list _effects;
  particle_list _particles;
  particle_list _to_add;
  std::mutex mutex_;
  float _wrap_x;
  float _wrap_z;

  // when you call remove_effect, we add it to this list and remove it
  // on the next loop through particle_manager::update()
  std::vector<particle_effect *> _dead_effects;

public:
  particle_manager();
  ~particle_manager();

  void initialize(Graphics *g);
  void update(float dt);
  void render(sg::scenegraph &scenegraph);

  // created the named effect (we load the properties from the given .wwpart file)
  std::shared_ptr<particle_effect> create_effect(std::string const &name);

  // this is called by the particle_effect when it's finished.
  void remove_effect(particle_effect *effect);

  // sets the wrapping of the world, once we get > (x, *, z) we wrap back to (0, *, 0).
  void set_world_wrap(float x, float z);
  float get_wrap_x() const {
    return _wrap_x;
  }
  float get_wrap_z() const {
    return _wrap_z;
  }

  /** Gets a count of the number of active (alive) particles, mostly for debugging. */
  long get_num_active_particles() const;

  // this is called by the particle_emitter to add a new particle
  void add_particle(particle *p);
};

}
