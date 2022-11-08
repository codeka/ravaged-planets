#include <framework/particle.h>
#include <framework/particle_config.h>
#include <framework/texture.h>
#include <framework/logging.h>
#include <framework/misc.h>

namespace fw {

//-------------------------------------------------------------------------
Particle::LifeState::LifeState() :
    age(0), size(0), color_row(0), alpha(0), rotation_speed(0), rotation(ParticleRotation::kRandom), speed(0),
    direction(fw::Vector(0, 0, 0)), gravity(0) {
}

Particle::LifeState::LifeState(Particle::LifeState const &copy) {
  age = copy.age;
  size = copy.size;
  color_row = copy.color_row;
  alpha = copy.alpha;
  rotation_speed = copy.rotation_speed;
  rotation = copy.rotation;
  speed = copy.speed;
  direction = copy.direction;
  gravity = copy.gravity;
}

//-------------------------------------------------------------------------

Particle::Particle() :
    rotation(ParticleRotation::kRandom), alpha(0), color1(0), color2(0), age(0), max_age_(0),
    color_factor(0), pos(0, 0, 0), direction(0, 0, 0) {
}

Particle::~Particle() {
}

void Particle::initialize(std::shared_ptr<ParticleEmitterConfig> const& config) {
  this->config = config;
  max_age_ = config->max_age.get_value();
  random = fw::random();
  alpha = 0;
  color1 = 0;
  color2 = 0;
  color_factor = 0.f;
  age = 0.0f;
  angle = 0.0f;
  size = 0.0f;
  draw_frame = 0;

  states_.clear();
  for (auto it = config->life.begin(); it != config->life.end(); it++) {
    LifeState state;
    state.age = (*it).age;
    state.size = (*it).size.get_value();
    state.color_row = (*it).color_row;
    state.alpha = (*it).alpha;
    state.rotation_speed = (*it).rotation_speed.get_value();
    state.rotation = (*it).rotation;
    state.speed = (*it).speed.get_value();
    state.gravity = (*it).gravity.get_value();
    state.direction = (*it).direction.get_value();
    if (state.direction.length() > 0.001f) {
      state.direction.normalize();
    }
    states_.push_back(state);
  }

  if (config->billboard.areas.size() < 1) {
    rect.left = rect.top = 0.0f;
    rect.width = rect.height = 1.0f;
  } else {
    int index = static_cast<int>(random * (config->billboard.areas.size() - 1));
    rect = config->billboard.areas[index];
  }

  direction = fw::Vector(fw::random() - 0.5f, fw::random() - 0.5f, fw::random() - 0.5f).normalized();
  pos = config->position.get_point();
}

bool Particle::update(float dt) {
  float ndt = dt / max_age_;
  age += ndt;
  if (age > 1.0f) {
    return false;
  }

  std::vector<LifeState>::iterator prev_it;
  std::vector<LifeState>::iterator next_it;
  for (prev_it = states_.begin(); prev_it != states_.end(); ++prev_it) {
    next_it = prev_it + 1;

    if (next_it == states_.end() || (*next_it).age > age) {
      break;
    }
  }

  float t;
  LifeState const *next;
  LifeState const *prev;

  if (next_it == states_.end()) {
    next = &(*prev_it);
    prev = &(*prev_it);
    t = 0.0f;
  } else {
    next = &(*next_it);
    prev = &(*prev_it);

    // lerp between (last_it) and (next_it)
    float age_diff = next->age - prev->age;

    // t is a value between 0 and 1. when age == last_it.age, t will be
    // zero. When age == next_it.age, t will be one.
    t = (age - prev->age) / age_diff;
  }

  // adjust the Particle's direction based on it's gravity factor
  fw::Vector gravity = fw::Vector(0, -1, 0) * (fw::lerp(prev->gravity, next->gravity, t));
  if (prev->direction.length() < 0.001f || next->direction.length() < 0.001f) {
    direction += gravity * dt;
  } else {
    direction = fw::lerp(prev->direction, next->direction, t);
    direction += gravity * age;
    direction.normalize();
  }

  float rotation_speed = 0.0f;
  rotation = prev->rotation;
  if (rotation == ParticleRotation::kRandom) {
    rotation_speed = fw::lerp(prev->rotation_speed, next->rotation_speed, t);
    angle += rotation_speed * dt;
  }

  float speed = fw::lerp(prev->speed, next->speed, t);
  color1 = prev->color_row;
  color2 = next->color_row;
  color_factor = t;
  alpha = fw::lerp(prev->alpha, next->alpha, t);
  size = fw::lerp(prev->size, next->size, t);

  // Don't update pos in the update thread, set new_pos and let the render thread update pos.
  pos += (direction * speed) * dt;
  return true;
}

}
