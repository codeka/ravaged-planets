#pragma once

#include <queue>
#include <boost/signals2.hpp>

#include <framework/lua.h>

#include <game/entities/entity.h>

namespace ent {
class ParticleEffectComponent;

/**
 * This component is attached to entities who have the ability to build things (units, buildings, etc).
 */
class BuilderComponent : public EntityComponent, public boost::signals2::trackable {
private:
  struct QueueEntry {
    fw::lua::Value tmpl;
    float time_to_build;
    float time_remaining;
    float percent_complete;

    QueueEntry(const fw::lua::Value& tmpl) : tmpl(tmpl) {}
  };
  ParticleEffectComponent *_particle_effect_component;
  std::string build_group_;
  std::queue<QueueEntry> _build_queue;

  void on_selected(bool selected);

public:
  static const int identifier = 250;
  virtual int get_identifier() {
    return identifier;
  }

  BuilderComponent();
  ~BuilderComponent();

  void build(std::string name);
  bool is_building() const;

  void apply_template(fw::lua::Value tmpl) override;
  virtual void initialize();
  virtual void update(float dt);
};

}
