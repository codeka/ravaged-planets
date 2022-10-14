#pragma once

#include <atomic>

#include <framework/vector.h>
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

  // If an effect marked destroy_entity_on_complete finishes, this will get set to true to signify that we need to
  // destroy the entity now.
  std::atomic<bool> queue_destroy_entity_;

public:
  static const int identifier = 700;

  ParticleEffectComponent();
  virtual ~ParticleEffectComponent();

  void apply_template(fw::lua::Value tmpl) override;

  virtual void initialize();
  virtual void update(float dt);

  void start_effect(std::string const &name);
  void stop_effect(std::string const &name);

  virtual int get_identifier() {
    return identifier;
  }
};

}
