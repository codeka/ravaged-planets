#pragma once

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace ent {
class pathing_component;
class position_component;

// this component is added to entities which can move (e.g. units, etc)
class moveable_component: public entity_component {
private:
  position_component *_position_component;
  pathing_component *_pathing_component;
  fw::Vector _intermediate_goal;
  fw::Vector _goal;
  float _speed;
  float _turn_speed;
  bool _avoid_collisions;
  bool _is_moving;

  fw::Vector steer(fw::Vector pos, fw::Vector curr_direction,
      fw::Vector goal_direction, float turn_amount, bool show_steering);

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

  void set_goal(fw::Vector goal, bool skip_pathing = false);
  fw::Vector get_goal() const {
    return _goal;
  }

  /**
   * Sets the "intermediate" goal, which is the location we're moving directly towards.
   *
   * The "goal" will be the final location along a path that was a travelling (via our pathing_component, if we
   * have one), and the pathing_component will set intermediate goals as we go.
   */
  void set_intermediate_goal(fw::Vector goal);
  fw::Vector get_intermediate_goal() const {
    return _intermediate_goal;
  }

  // Stop moving.
  void stop();
};

}
