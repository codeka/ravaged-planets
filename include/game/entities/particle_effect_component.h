#pragma once

#include <game/entities/entity.h>

namespace fw {
class particle_effect;
}

namespace ent {
class position_component;

// this is a component that holds a particle_emitter and allows an entity
// to act as a particle emitter as well (basically, the particle_emitter
// follows the entity around)
class particle_effect_component: public entity_component {
private:
  std::string _effect_name;
  std::shared_ptr<fw::particle_effect> _effect;
  position_component *_our_position;
  bool _destroy_entity_on_complete;

public:
  static const int identifier = 700;

  particle_effect_component();
  virtual ~particle_effect_component();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void initialize();
  virtual void update(float dt);
  virtual void render(fw::sg::scenegraph &, fw::matrix const &);

  virtual int get_identifier() {
    return identifier;
  }
};

}
