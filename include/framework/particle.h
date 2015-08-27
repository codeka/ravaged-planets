#pragma once

#include <memory>
#include <vector>

#include <framework/vector.h>
#include <framework/colour.h>

namespace fw {
class texture;
class particle_emitter_config;

namespace rotation_kind {
enum value {
  random, direction
};
}

/**
 * This simply controls the top,left and bottom,right corners of the billboard texture we apply for a given particle.
 * By default, it's 0,0 1,1.
 */
struct billboard_rect {
  float left, top, right, bottom;
};

/** Represents a single particle. */
class particle {
public:
  struct life_state {
    float age;
    float size;
    float rotation_speed;
    rotation_kind::value rotation_kind;
    float alpha;
    float speed;
    fw::vector direction;
    float gravity;
    int colour_row;

    life_state();
    life_state(life_state const &copy);
  };

private:
  float _max_age;
  std::vector<life_state> _states;

public:
  std::shared_ptr<particle_emitter_config> config;
  fw::vector direction;
  fw::vector pos;
  int colour1;
  int colour2;
  float colour_factor;
  float alpha;
  float size;
  float age;
  float angle;
  rotation_kind::value rotation_kind;
  billboard_rect rect;

  /**
   * The renderer wants to make sure it only draws each particle once per frame, so this is a number that is
   * incrememnted each frame and compared when we go to draw the particle. If if the same, we know we've already
   * drawn this particle this frame.
   */
  int draw_frame;

  /** A random number between 0 and 1 that we can use to calculate various characteristics of this particle's life. */
  float random;

  particle(std::shared_ptr<particle_emitter_config> const &config);
  ~particle();

  void initialize();
  bool update(float dt);
};

}
