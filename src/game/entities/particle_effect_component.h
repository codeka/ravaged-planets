#pragma once

#include <atomic>

#include <framework/math.h>

#include <game/entities/entity.h>

namespace fw {
class ParticleEffect;
}

namespace ent {
class PositionComponent;

// This component holds a bunch of ParticleEmitters, and allows an Entity to act as a particle emitter as well
// (basically, the ParticleEmitter follows the Entity around).
class ParticleEffectComponent: public EntityComponent {
private:
  struct EffectInfo {
    std::string name;
    std::shared_ptr<fw::ParticleEffect> effect;
    fw::Vector offset;
    bool destroy_entity_on_complete;
    bool started;

    EffectInfo() : destroy_entity_on_complete(false), started(false) {
    }
  };

  std::map<std::string, EffectInfo> effects_;
  PositionComponent *our_position_;

public:
  static const int identifier = 700;

  ParticleEffectComponent();
  virtual ~ParticleEffectComponent();

  void apply_template(fw::lua::Value tmpl) override;

  void initialize() override;
  void update(float dt) override;

  void start_effect(std::string const &name);
  void stop_effect(std::string const &name);

  virtual int get_identifier() {
    return identifier;
  }
};

}
