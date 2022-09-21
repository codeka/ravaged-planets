
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

ParticleManager::ParticleManager() :
    renderer_(nullptr), graphics_(nullptr), wrap_x_(0.0f), wrap_z_(0.0f) {
  renderer_ = new ParticleRenderer(this);
}

ParticleManager::~ParticleManager() {
  delete renderer_;
}

void ParticleManager::initialize(Graphics *g) {
  graphics_ = g;
  renderer_->initialize(g);
}

void ParticleManager::set_world_wrap(float x, float z) {
  wrap_x_ = x;
  wrap_z_ = z;
}

void ParticleManager::update(float dt) {
  for (std::vector<ParticleEffect *>::iterator dit = dead_effects_.begin(); dit != dead_effects_.end(); dit++) {
    ParticleEffect *effect = *dit;
    for (EffectList::iterator it = effects_.begin(); it != effects_.end(); it++) {
      if ((*it).get() == effect) {
        effects_.erase(it);
        break;
      }
    }
  }
  dead_effects_.clear();

  for (EffectList::iterator it = effects_.begin(); it != effects_.end(); ++it) {
    (*it)->update(dt);
  }
}

void ParticleManager::render(sg::scenegraph &scenegraph) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for(Particle *p : to_add_) {
      particles_.push_back(p);
    }
    to_add_.clear();
  }

  renderer_->render(scenegraph, particles_);

  // remove any dead particles
  particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](Particle const *p) {
    return p->age >= 1.0f;
  }), particles_.end());
}

long ParticleManager::get_num_active_particles() const {
  // some may have died but not been removed yet, that's OK as this is just to give us an idea.
  return particles_.size();
}

std::shared_ptr<ParticleEffect> ParticleManager::create_effect(std::string const &name) {
  FW_ENSURE_RENDER_THREAD();

  auto config = ParticleEffectConfig::load(name);
  std::shared_ptr <ParticleEffect> effect(new ParticleEffect(this, config));
  effect->initialize();
  effects_.push_back(effect);
  return effect;
}

void ParticleManager::remove_effect(ParticleEffect *effect) {
  dead_effects_.push_back(effect);
}

/**
 * Because this is called on the update thread, we don't add directly to the particles_ list until the render thread.
 */
void ParticleManager::add_particle(Particle *p) {
  std::unique_lock<std::mutex> lock(mutex_);
  to_add_.push_back(p);
}

}
