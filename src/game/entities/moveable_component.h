#pragma once

#include <framework/vector.h>
#include <game/entities/entity.h>

namespace ent {
class PathingComponent;
class PositionComponent;

// this component is added to entities which can move (e.g. units, etc)
class MoveableComponent: public EntityComponent {
private:
  PositionComponent *position_component_;
  PathingComponent *pathing_component_;
  fw::Vector intermediate_goal_;
  fw::Vector goal_;
  float speed_;
  float turn_speed_;
  bool avoid_collisions_;
  bool is_moving_;

  fw::Vector steer(fw::Vector pos, fw::Vector curr_direction,
      fw::Vector goal_direction, float turn_amount, bool show_steering);

public:
  static const int identifier = 400;
  virtual int get_identifier() {
    return identifier;
  }

  MoveableComponent();
  ~MoveableComponent();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void initialize();
  virtual void update(float dt);

  void set_speed(float speed) {
    speed_ = speed;
  }
  void set_turn_speed(float turn_speed) {
    turn_speed_ = turn_speed;
  }
  void set_avoid_collisions(bool ParticleRotation) {
    avoid_collisions_ = ParticleRotation;
  }

  void set_goal(fw::Vector goal, bool skip_pathing = false);
  fw::Vector get_goal() const {
    return goal_;
  }

  // Sets the "intermediate" goal, which is the location we're moving directly towards.
  //
  // The "goal" will be the final location along a path that was a travelling (via our pathing_component, if we
  // have one), and the pathing_component will set intermediate goals as we go.
  void set_intermediate_goal(fw::Vector goal);
  fw::Vector get_intermediate_goal() const {
    return intermediate_goal_;
  }

  // Stop moving.
  void stop();
};

}
