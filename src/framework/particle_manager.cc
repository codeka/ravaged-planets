
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
    renderer_(nullptr), graphics_(nullptr), wrap_x_(0.0f), wrap_z_(0.0f), particle_pool_(/*initial_size=*/1000) {
  renderer_ = new ParticleRenderer(this);
  auto* renderer = renderer_;
  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [renderer](fw::sg::Scenegraph& scenegraph) {
      scenegraph.add_callback(renderer);
    });
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
  // TODO: remove this method, all particle stuff runs on the render thread
}

ParticleManager::ParticleList& ParticleManager::on_render(float dt) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (EffectList::iterator it = effects_.begin(); it != effects_.end(); ++it) {
      (*it)->update(dt);
    }
  }

  // remove any dead particles
  particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const std::weak_ptr<Particle> &p) {
    // If the underlying shared_ptr is gone, we remove it.
    return p.expired();
  }), particles_.end());

  // Return the particles so that the renderer can render them.
  return particles_;
}

long ParticleManager::get_num_active_particles() const {
  // some may have died but not been removed yet, that's OK as this is just to give us an idea.
  return particles_.size();
}

std::shared_ptr<ParticleEffect> ParticleManager::create_effect(
    std::string const &name, const fw::Vector& initial_position) {
  auto config = ParticleEffectConfig::load(name);
  auto effect = std::make_shared<ParticleEffect>(this, particle_pool_, config, initial_position);

  {
    std::unique_lock<std::mutex> lock(mutex_);
    effects_.push_back(effect);
  }

  return effect;
}

void ParticleManager::remove_effect(const std::shared_ptr<ParticleEffect>& effect) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto it = effects_.begin(); it != effects_.end(); ++it) {
    if (it->get() == effect.get()) {
      effects_.erase(it);
      return;
    }
  }
}

void ParticleManager::add_particle(std::weak_ptr<Particle> p) {
  FW_ENSURE_RENDER_THREAD();

  particles_.push_back(p);
}

}
