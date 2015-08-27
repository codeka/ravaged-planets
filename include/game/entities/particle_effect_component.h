#pragma once

#include <game/entities/entity.h>

namespace fw {
class particle_effect;
}

namespace ent {
class position_component;

/**
 * This component holds a bunch of particle_emitters, and allows an entity to act as a particle emitter as well
 * (basically, the particle_emitter follows the entity around).
 */
class particle_effect_component: public entity_component {
private:
  struct effect_info {
    std::string name;
    std::shared_ptr<fw::particle_effect> effect;
    bool destroy_entity_on_complete;
    bool started;

    effect_info() : destroy_entity_on_complete(false), started(false) {
    }
  };

  std::map<std::string, effect_info> _effects;
  position_component *_our_position;

public:
  static const int identifier = 700;

  particle_effect_component();
  virtual ~particle_effect_component();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void initialize();
  virtual void update(float dt);
  virtual void render(fw::sg::scenegraph &, fw::matrix const &);

  void start_effect(std::string const &name);
  void stop_effect(std::string const &name);

  virtual int get_identifier() {
    return identifier;
  }
};

}
