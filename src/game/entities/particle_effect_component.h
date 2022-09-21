#pragma once

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace fw {
class ParticleEffect;
}

namespace ent {
class position_component;

/**
 * This component holds a bunch of particle_emitters, and allows an entity to act as a Particle emitter as well
 * (basically, the ParticleEmitter follows the entity around).
 */
class particle_effect_component: public entity_component {
private:
  struct effect_info {
    std::string name;
    std::shared_ptr<fw::ParticleEffect> effect;
    fw::Vector offset;
    bool destroy_entity_on_complete;
    bool started;

    effect_info() : destroy_entity_on_complete(false), started(false) {
    }
  };

  std::map<std::string, effect_info> effects_;
  position_component *_our_position;

public:
  static const int identifier = 700;

  particle_effect_component();
  virtual ~particle_effect_component();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void initialize();
  virtual void update(float dt);
  virtual void render(fw::sg::Scenegraph &, fw::Matrix const &);

  void start_effect(std::string const &name);
  void stop_effect(std::string const &name);

  virtual int get_identifier() {
    return identifier;
  }
};

}
