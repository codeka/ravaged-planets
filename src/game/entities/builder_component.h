#pragma once

#include <queue>
#include <boost/signals2.hpp>

#include <game/entities/entity.h>

namespace ent {
class entity_template;
class particle_effect_component;

/**
 * This component is attached to entities who have the ability to build things (units, buildings, etc).
 */
class builder_component: public entity_component, public boost::signals2::trackable {
private:
  struct queue_entry {
    luabind::object tmpl;
    float time_to_build;
    float time_remaining;
    float percent_complete;
  };
  particle_effect_component *_particle_effect_component;
  std::string _build_group;
  std::queue<queue_entry> _build_queue;

  void on_selected(bool selected);

public:
  static const int identifier = 250;
  virtual int get_identifier() {
    return identifier;
  }

  builder_component();
  ~builder_component();

  void build(std::string name);
  bool is_building() const;

  virtual void apply_template(luabind::object const &tmpl);
  virtual void initialize();
  virtual void update(float dt);
};

}
