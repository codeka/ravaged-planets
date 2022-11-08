#pragma once

#include <mutex>

#include <framework/math.h>

#include <game/entities/entity.h>

namespace ent {
class MoveableComponent;
class PositionComponent;

// The pathing component is attached to each Entity that will follow a path
// over the terrain (for example, tanks have this; helicopters do not)
class PathingComponent: public EntityComponent {
private:
  std::mutex mutex_;

  fw::Vector last_request_goal_;
  float last_request_time_;
  size_t curr_goal_node_;
  std::vector<fw::Vector> path_;
  PositionComponent *position_;
  MoveableComponent *moveable_;

  // This is called on the pathing thread, so we just set new_path_ (locked by mutex_) and then update path_ in
  // the update method.
  void on_path_found(std::vector<fw::Vector> const &path);
  std::vector<fw::Vector> new_path_;

public:
  static const int identifier = 650;
  virtual int get_identifier() {
    return identifier;
  }

  PathingComponent();
  ~PathingComponent();

  virtual void initialize();
  virtual void update(float dt);

  // sets the goal for this Entity. we request the path from the pathing_thread and when it comes back, we'll start
  // moving along it.
  void set_goal(fw::Vector const &goal);

  // If we're following a path (or waiting for a path), stop now.
  void stop();

  // get a flag that simply indicates whether we're currently following a path or not
  bool is_following_path() const;
};

}
