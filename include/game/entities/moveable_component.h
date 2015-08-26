#pragma once

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace ent {
class position_component;

// this component is added to entities which can move (e.g. units, etc)
class moveable_component: public entity_component {
private:
  fw::vector _goal;
  position_component *_pos;
  float _speed;
  float _turn_speed;
  bool _avoid_collisions;

  fw::vector steer(fw::vector pos, fw::vector curr_direction,
      fw::vector goal_direction, float turn_amount, bool show_steering);

public:
  static const int identifier = 400;
  virtual int get_identifier() {
    return identifier;
  }

  moveable_component();
  ~moveable_component();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void initialize();
  virtual void update(float dt);

  void set_speed(float speed) {
    _speed = speed;
  }
  void set_turn_speed(float turn_speed) {
    _turn_speed = turn_speed;
  }
  void set_avoid_collisions(bool value) {
    _avoid_collisions = value;
  }

  void set_goal(fw::vector goal);
  fw::vector get_goal() const {
    return _goal;
  }
};

}
