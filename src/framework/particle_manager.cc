
#include <boost/foreach.hpp>

#include <framework/particle_manager.h>
#include <framework/particle_emitter.h>
#include <framework/particle_effect.h>
#include <framework/particle_config.h>
#include <framework/particle.h>
#include <framework/particle_renderer.h>
#include <framework/framework.h>
#include <framework/timer.h>
#include <framework/scenegraph.h>

namespace fw {

particle_manager::particle_manager() :
    _renderer(nullptr), _graphics(nullptr), _wrap_x(0.0f), _wrap_z(0.0f) {
  _renderer = new particle_renderer(this);
}

particle_manager::~particle_manager() {
  delete _renderer;
}

void particle_manager::initialize(Graphics *g) {
  _graphics = g;
  _renderer->initialize(g);
}

void particle_manager::set_world_wrap(float x, float z) {
  _wrap_x = x;
  _wrap_z = z;
}

void particle_manager::update(float dt) {
  for (std::vector<particle_effect *>::iterator dit = _dead_effects.begin(); dit != _dead_effects.end(); dit++) {
    particle_effect *effect = *dit;
    for (effect_list::iterator it = _effects.begin(); it != _effects.end(); it++) {
      if ((*it).get() == effect) {
        _effects.erase(it);
        break;
      }
    }
  }
  _dead_effects.clear();

  for (effect_list::iterator it = _effects.begin(); it != _effects.end(); ++it) {
    (*it)->update(dt);
  }
}

void particle_manager::render(sg::scenegraph &scenegraph) {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    BOOST_FOREACH(particle *p, _to_add) {
      _particles.push_back(p);
    }
    _to_add.clear();
  }

  _renderer->render(scenegraph, _particles);

  // remove any dead particles
  _particles.erase(std::remove_if(_particles.begin(), _particles.end(), [](particle const *p) {
    return p->age >= 1.0f;
  }), _particles.end());
}

long particle_manager::get_num_active_particles() const {
  // some may have died but not been removed yet, that's OK as this is just to give us an idea.
  return _particles.size();
}

std::shared_ptr<particle_effect> particle_manager::create_effect(std::string const &name) {
  FW_ENSURE_RENDER_THREAD();

  std::shared_ptr<particle_effect_config> config = particle_effect_config::load(name);
  std::shared_ptr <particle_effect> effect(new particle_effect(this, config));
  effect->initialize();
  _effects.push_back(effect);
  return effect;
}

void particle_manager::remove_effect(particle_effect *effect) {
  _dead_effects.push_back(effect);
}

/**
 * Because this is called on the update thread, we don't add directly to the _particles list until the render thread.
 */
void particle_manager::add_particle(particle *p) {
  std::unique_lock<std::mutex> lock(_mutex);
  _to_add.push_back(p);
}

}
