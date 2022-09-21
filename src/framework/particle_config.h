#pragma once

#include <memory>
#include <vector>

#include <framework/vector.h>
#include <framework/color.h>
#include <framework/particle.h>
#include <framework/texture.h>
#include <framework/xml.h>

namespace fw {
class EmitPolicy;
class ParticleEmitterConfig;

/**
 * particle_effect_config represents a collection of particle_emitter_configs, one for each emitter the corresponding
 * effect represents.
 */
class ParticleEffectConfig {
public:
  typedef std::vector<std::shared_ptr<ParticleEmitterConfig>> EmitterConfigList;
private:
  EmitterConfigList emitter_configs_;

  ParticleEffectConfig();

  bool load_document(xml_element const &root);
  void load_emitter(xml_element const &elem);

public:
  static std::shared_ptr<ParticleEffectConfig> load(std::string const &name);

  inline EmitterConfigList::iterator emitter_config_begin() {
    return emitter_configs_.begin();
  }
  inline EmitterConfigList::const_iterator emitter_config_begin() const {
    return emitter_configs_.begin();
  }

  inline EmitterConfigList::iterator emitter_config_end() {
    return emitter_configs_.end();
  }
  inline EmitterConfigList::const_iterator emitter_config_end() const {
    return emitter_configs_.end();
  }
};

// This class represents the configuration for a Particle emitter within an effect.
class ParticleEmitterConfig {
public:
  friend class ParticleEffectConfig;

  // Represents the different kinds of falloff that is possible when we're talking about something centered at
  // a point with a radius.
  enum FalloffKind {
    // A constant fall-off. we choose a spot in the sphere with an equal probability for anywhere in the sphere.
    kConstant,

    // A linear falloff. we choose the center of the sphere with a high probability, falling off linearly to the
    // circumference, which has a zero probability.
    kLinear,

    // An exponential fallout. we choose the center of the sphere with a very high probability, falling off
    // exponentially to the circumference, which has a zero probability.
    kExponential
  };

  /**
   * This defines a spherical area where we choose a point inside the sphere with a certain probability.
   */
  struct SphericalLocation {
    Vector center;
    float radius;
    FalloffKind falloff;

    // Gets a point within this location, using the probability rules to work out exactly where to go.
    Vector get_point() const;
  };

  // This controls the technique we use in the Particle.fx file for the texture.
  enum BillboardMode {
    kNormal, kAdditive
  };

  // Represents the base properties of the texture for this Particle effect.
  struct BillboardTexture {
    std::shared_ptr<fw::Texture> texture;

    // The "billboard mode" defines which technique from the Particle.fx file to use.
    BillboardMode mode;

    /**
     * These are the (u,v) coords of the top/left and bottom/right of the part of the texture we get our values from,
     * we use (0,0) and (1,1) by default we choose a Random one for a given Particle - each one is basically a
     * "variant" of the texture we'll use.
     */
    std::vector<Rectangle<float>> areas;
  };

  // Represents a Random ParticleRotation between a specified min and max ParticleRotation.
  template<typename T>
  struct Random {
    T min;
    T max;

    // gets a Random ParticleRotation between min and max
    inline T get_value() const {
      return min + ((max - min) * fw::random());
    }
  };

  struct LifeState {
    float age;
    Random<float> size;
    Random<float> rotation_speed;
    ParticleRotation rotation;
    Random<fw::Vector> direction;
    Random<float> speed;
    int color_row;
    float alpha;
    Random<float> gravity;

    LifeState();
    LifeState(LifeState const &copy);
  };

private:
  ParticleEmitterConfig();

  void load_emitter(xml_element const &elem);
  void load_position(xml_element const &elem);
  void load_billboard(xml_element const &elem);
  void load_life(xml_element const &elem);
  void load_emit_policy(xml_element const &elem);
  void parse_life_state(LifeState &state, xml_element const &elem);
  void parse_random_float(Random<float> &ParticleRotation, xml_element const &elem);
  void parse_random_color(Random<fw::Color> &ParticleRotation, xml_element const &elem);
  void parse_random_vector(Random<fw::Vector> &ParticleRotation, xml_element const &elem);

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
  SphericalLocation position;

  /** This is the properties of the billboard texture we'll use to display this Particle. */
  BillboardTexture billboard;

  /**
   * This is the maximum age we'll allow for a Particle to exist. The ParticleRotation is Random, and it causes ALL "age-based"
   * calculations to be randomized as well.
   */
  Random<float> max_age;

  // this is the list of LifeState instances for this Particle. Each state contains
  // things like the size of the Particle at that point of time, the color and so
  // on. As the Particle ages, we want to lerp between each LifeState to provide
  // smooth Particle effects.
  std::vector<LifeState> life;

  // this is the "policy" we use for deciding when to emit a new Particle.
  std::string emit_policy_name;
  float emit_policy_value;
};

}
