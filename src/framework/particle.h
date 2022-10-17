#pragma once

#include <memory>
#include <vector>

#include <framework/color.h>
#include <framework/misc.h>
#include <framework/texture.h>
#include <framework/vector.h>

namespace fw {
class ParticleEmitterConfig;

enum ParticleRotation {
  kRandom, kDirection
};

// Represents a single particle.
class Particle {
public:
  struct LifeState {
    float age;
    float size;
    float rotation_speed;
    ParticleRotation rotation;
    float alpha;
    float speed;
    fw::Vector direction;
    float gravity;
    int color_row;

    LifeState();
    LifeState(LifeState const &copy);
  };

private:
  float max_age_;
  std::vector<LifeState> states_;

public:
  std::shared_ptr<ParticleEmitterConfig> config;
  fw::Vector direction;
  fw::Vector pos;
  int color1;
  int color2;
  float color_factor;
  float alpha;
  float size;
  float age;
  float angle;
  ParticleRotation rotation;
  Rectangle<float> rect;

  // The renderer wants to make sure it only draws each Particle once per frame, so this is a number that is
  // incrememnted each frame and compared when we go to draw the Particle. If if the same, we know we've already
  // drawn this Particle this frame.
  int draw_frame;

  // A random number between 0 and 1 that we can use to calculate various characteristics of this Particle's life. 
  float random;

  Particle();
  ~Particle();

  void initialize(std::shared_ptr<ParticleEmitterConfig> const& config);
  bool update(float dt);
};

}
