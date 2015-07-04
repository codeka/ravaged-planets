#pragma once

#include <game/entities/entity.h>
#include <framework/vector.h>

namespace ent {
class moveable_component;
class position_component;

// The pathing component is attached to each entity that will follow a path
// over the terrain (for example, tanks have this; helicopters do not)
class pathing_component: public entity_component {
private:
  size_t _curr_goal_node;
  std::vector<fw::vector> _path;
  position_component *_position;
  moveable_component *_moveable;

  void on_path_found(std::vector<fw::vector> const &path);

public:
  static const int identifier = 650;
  virtual int get_identifier() {
    return identifier;
  }

  pathing_component();
  ~pathing_component();

  virtual void initialize();
  virtual void update(float dt);

  // sets the path that we're to follow until we reach the goal
  void set_path(std::vector<fw::vector> const &path);

  // sets the goal for this entity. we request the path from the pathing_thread
  // and when it comes back, we'll start moving along it.
  void set_goal(fw::vector const &goal);

  // get a flag that simply indicates whether we're currently following a path or not
  bool is_following_path() const;
};

}
