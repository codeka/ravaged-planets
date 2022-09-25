#pragma once

#include <game/entities/entity.h>
#include <framework/vector.h>

namespace ent {
class MoveableComponent;
class PositionComponent;

// The pathing component is attached to each Entity that will follow a path
// over the terrain (for example, tanks have this; helicopters do not)
class PathingComponent: public EntityComponent {
private:
  fw::Vector last_request_goal_;
  float last_request_time_;
  size_t curr_goal_node_;
  std::vector<fw::Vector> path_;
  PositionComponent *position_;
  MoveableComponent *moveable_;

  void on_path_found(std::vector<fw::Vector> const &path);

public:
  static const int identifier = 650;
  virtual int get_identifier() {
    return identifier;
  }

  PathingComponent();
  ~PathingComponent();

  virtual void initialize();
  virtual void update(float dt);

  // sets the path that we're to follow until we reach the goal
  void set_path(std::vector<fw::Vector> const &path);

  // sets the goal for this Entity. we request the path from the pathing_thread and when it comes back, we'll start
  // moving along it.
  void set_goal(fw::Vector const &goal);

  // If we're following a path (or waiting for a path), stop now.
  void stop();

  // get a flag that simply indicates whether we're currently following a path or not
  bool is_following_path() const;
};

}
