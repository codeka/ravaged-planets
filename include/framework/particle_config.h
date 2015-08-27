#pragma once

#include <memory>
#include <vector>

#include <framework/vector.h>
#include <framework/colour.h>
#include <framework/particle.h>

namespace fw {
class texture;
class xml_element;
class emit_policy;
class particle_emitter_config;

/**
 * particle_effect_config represents a collection of particle_emitter_configs, one for each emitter the corresponding
 * effect represents.
 */
class particle_effect_config {
public:
  typedef std::vector<std::shared_ptr<particle_emitter_config>> emitter_config_list;
private:
  emitter_config_list _emitter_configs;

  particle_effect_config();

  bool load_document(xml_element const &root);
  void load_emitter(xml_element const &elem);

public:
  static std::shared_ptr<particle_effect_config> load(std::string const &name);

  inline emitter_config_list::iterator emitter_config_begin() {
    return _emitter_configs.begin();
  }
  inline emitter_config_list::const_iterator emitter_config_begin() const {
    return _emitter_configs.begin();
  }

  inline emitter_config_list::iterator emitter_config_end() {
    return _emitter_configs.end();
  }
  inline emitter_config_list::const_iterator emitter_config_end() const {
    return _emitter_configs.end();
  }
};

/** This class represents the configuration for a particle emitter within an effect. */
class particle_emitter_config {
public:
  friend class particle_effect_config;

  /**
   * Represents the different kinds of falloff that is possible when we're talking about something centered at
   * a point with a radius.
   */
  enum falloff_kind {
    /** A constant fall-off. we choose a spot in the sphere with an equal probability for anywhere in the sphere. */
    constant,

    /**
     * A linear falloff. we choose the centre of the sphere with a high probability, falling off linearly to the
     * circumference, which has a zero probability.
     */
    linear,

    /**
     * An exponential fallout. we choose the centre of the sphere with a very high probability, falling off
     * exponentially to the circumference, which has a zero probability.
     */
    exponential
  };

  /**
   * This defines a spherical area where we choose a point inside the sphere with a certain probability.
   */
  struct spherical_location {
    vector centre;
    float radius;
    falloff_kind falloff;

    /** Gets a point within this location, using the probability rules to work out exactly where to go. */
    vector get_point() const;
  };

  /** This controls the technique we use in the particle.fx file for the texture. */
  enum billboard_mode {
    normal, additive
  };

  /** Represents the base properties of the texture for this particle effect. */
  struct billboard_texture {
    std::shared_ptr<fw::texture> texture;

    /** The "billboard mode" defines which technique from the particle.fx file to use. */
    billboard_mode mode;

    /**
     * These are the (u,v) coords of the top/left and bottom/right of the part of the texture we get our values from,
     * we use (0,0) and (1,1) by default we choose a random one for a given particle - each one is basically a
     * "variant" of the texture we'll use.
     */
    std::vector<billboard_rect> areas;
  };

  /** Represents a random value between a specified min and max value. */
  template<typename T>
  struct random {
    T min;
    T max;

    // gets a random value between min and max
    inline T get_value() const {
      return min + ((max - min) * fw::random());
    }
  };

  struct life_state {
    float age;
    random<float> size;
    random<float> rotation_speed;
    rotation_kind::value rotation_kind;
    random<fw::vector> direction;
    random<float> speed;
    int colour_row;
    float alpha;
    random<float> gravity;

    life_state();
    life_state(life_state const &copy);
  };

private:
  particle_emitter_config();

  void load_emitter(xml_element const &elem);
  void load_position(xml_element const &elem);
  void load_billboard(xml_element const &elem);
  void load_life(xml_element const &elem);
  void load_emit_policy(xml_element const &elem);
  void parse_life_state(life_state &state, xml_element const &elem);
  void parse_random_float(random<float> &value, xml_element const &elem);
  void parse_random_colour(random<fw::colour> &value, xml_element const &elem);
  void parse_random_vector(random<fw::vector> &value, xml_element const &elem);

public:

  /**
   * This is the start and end time for the emitter. the emitter will only emit particles after start_time seconds have
   * passed, and will kill itself after end_time seconds have passed (if end_time is zero, the emitter lasts until
   * the effect itself is destroyed)
   */
  float start_time;
  float end_time;

  /**
   * When the emitter starts up, this is the initial number of particles we'll emit immediately (you'd usually make
   * this relatively low). Useful for explosions and stuff which basically emit a bunch of particles and then die
   * immediately.
   */
  int initial_count;

  /** This is the initial position we choose for particles. */
  spherical_location position;

  /** This is the properties of the billboard texture we'll use to display this particle. */
  billboard_texture billboard;

  /**
   * This is the maximum age we'll allow for a particle to exist. The value is random, and it causes ALL "age-based"
   * calculations to be randomized as well.
   */
  random<float> max_age;

  // this is the list of life_state instances for this particle. Each state contains
  // things like the size of the particle at that point of time, the colour and so
  // on. As the particle ages, we want to lerp between each life_state to provide
  // smooth particle effects.
  std::vector<life_state> life;

  // this is the "policy" we use for deciding when to emit a new particle.
  std::string emit_policy_name;
  float emit_policy_value;
};

}
