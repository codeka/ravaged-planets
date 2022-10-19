#include <stack>

#include <framework/misc.h>
#include <framework/particle.h>
#include <framework/particle_config.h>
#include <framework/particle_emitter.h>
#include <framework/particle_manager.h>

namespace fw {

ParticleEmitter::ParticleEmitter(
    ParticleManager *mgr, ObjectPool<Particle>& particle_pool, std::shared_ptr<ParticleEmitterConfig> config,
    const fw::Vector& initial_position)
    : mgr_(mgr), particle_pool_(particle_pool), emit_policy_(0), config_(config), dead_(false), age_(0.0f),
      position_(initial_position) {
  if (config_->emit_policy_name == "distance") {
    emit_policy_ = new DistanceEmitPolicy(this, config_->emit_policy_value);
  } else if (config_->emit_policy_name == "timed") {
    emit_policy_ = new TimedEmitPolicy(this, config_->emit_policy_value);
  } else { // if (config_->emit_policy_name == "none")
    emit_policy_ = new NoEmitPolicy(this, config_->emit_policy_value);
  }

  initial_count_ = config_->initial_count;
}

ParticleEmitter::~ParticleEmitter() {
}

bool ParticleEmitter::update(float dt) {
  age_ += dt;

  // if we're not supposed to have started yet, don't do anything
  if (config_->start_time > age_)
    return true;

  if (!dead_ && config_->end_time > 0 && age_ > config_->end_time) {
    destroy();
  }

  // check whether we need to emit a new Particle
  if (!dead_) {
    while (initial_count_ > 0) {
      emit(get_position());
      initial_count_--;
    }

    emit_policy_->check_emit(dt);
  }

  // go through each Particle and update it's various properties
  for (ParticleList::iterator it = particles_.begin(); it != particles_.end();) {
    auto p = *it;
    if (!p->update(dt)) {
      // It's dead, so remove it.
      it = particles_.erase(it);
    } else {
      ++it;
    }
  }

  return (!dead_ || particles_.size() != 0);
}

void ParticleEmitter::destroy() {
  dead_ = true;
}

// This is called when it's time to emit a new Particle. The offset is used when emitting "extra" particles, we need
// to offset their age and position a bit.
std::shared_ptr<Particle> ParticleEmitter::emit(fw::Vector pos, float time_offset /*= 0.0f*/) {
  std::shared_ptr<Particle> p(particle_pool_.get_or_new());
  p->initialize(config_);
  p->pos = pos;
  p->age = time_offset;

  particles_.push_back(p);
  mgr_->add_particle(p);

  return p;
}

//-------------------------------------------------------------------------

TimedEmitPolicy::TimedEmitPolicy(ParticleEmitter* emitter, float ParticleRotation) :
    EmitPolicy(emitter), particles_per_second_(0.0f), time_since_last_particle_(0.0f), last_position_(0, 0, 0) {
  particles_per_second_ = ParticleRotation;
}

TimedEmitPolicy::~TimedEmitPolicy() {
}

void TimedEmitPolicy::check_emit(float dt) {
  float _seconds_per_particle = 1.0f / particles_per_second_;

  bool offset_pos = (time_since_last_particle_ != 0.0f);

  time_since_last_particle_ += dt;
  float time_offset = dt;

  fw::Vector pos = emitter_->get_position();
  fw::Vector dir = (pos - last_position_).normalize();

  while (time_since_last_particle_ > _seconds_per_particle) {
    time_offset -= _seconds_per_particle;
    time_since_last_particle_ -= _seconds_per_particle;

    fw::Vector curr_pos = offset_pos ? last_position_ + (dir * _seconds_per_particle) : pos;

    emitter_->emit(curr_pos, time_offset);
  }

  last_position_ = pos;
}

//-------------------------------------------------------------------------

DistanceEmitPolicy::DistanceEmitPolicy(ParticleEmitter* emitter, float max_distance)
  : EmitPolicy(emitter), max_distance_(0.0f) {
  max_distance_ = max_distance;
}

DistanceEmitPolicy::~DistanceEmitPolicy() {
}

void DistanceEmitPolicy::check_emit(float) {
  std::shared_ptr<Particle> last_particle = last_particle_.lock();
  if (!last_particle) {
    last_particle_ = emitter_->emit(emitter_->get_position());
    return;
  }

  float wrap_x = emitter_->get_manager()->get_wrap_x();
  float wrap_z = emitter_->get_manager()->get_wrap_z();

  fw::Vector next_pos = emitter_->get_position();
  fw::Vector last_pos = last_particle->pos;
  fw::Vector dir = get_direction_to(last_pos, next_pos, wrap_x, wrap_z).normalize();
  fw::Vector curr_pos = last_pos + (dir * max_distance_);

  float time_offset = 0.0f; // todo: this should be non-zero...

  float this_distance = calculate_distance(curr_pos, next_pos, wrap_x, wrap_z);
  float last_distance = this_distance + 1.0f;
  while (last_distance >= this_distance) {
    last_particle_ = emitter_->emit(curr_pos, time_offset);
    curr_pos += dir * max_distance_;

    last_distance = this_distance;
    this_distance = (curr_pos - next_pos).length();
  }
}

//-------------------------------------------------------------------------

NoEmitPolicy::NoEmitPolicy(ParticleEmitter* emitter, float): EmitPolicy(emitter) {
}

NoEmitPolicy::~NoEmitPolicy() {
}

void NoEmitPolicy::check_emit(float) {
  // we destroy ourselves because we're actually never going to
  // do anything
  emitter_->destroy();
}

}
