#include <functional>

#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/timer.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/pathing_component.h>
#include <game/entities/position_component.h>
#include <game/entities/moveable_component.h>
#include <game/world/world.h>
#include <game/ai/pathing_thread.h>

namespace ent {

using namespace std::placeholders;

// register the pathing component with the entity_factory
ENT_COMPONENT_REGISTER("Pathing", pathing_component);

pathing_component::pathing_component() :
    position_(nullptr), _moveable(nullptr), _curr_goal_node(0), _last_request_time(0.0f) {
}

pathing_component::~pathing_component() {
}

void pathing_component::initialize() {
  std::shared_ptr<entity> ent = entity_.lock();
  if (ent) {
    position_ = ent->get_component<position_component>();
    _moveable = ent->get_component<moveable_component>();
  }
}

void pathing_component::update(float dt) {
  // follow the path... todo: this can be done SOOOOOO much better!
  while (is_following_path()) {
    fw::Vector goal = _path[_curr_goal_node];
    fw::Vector dir = position_->get_direction_to(goal);
    dir[1] = 0.0f; // ignore height component
    if (dir.length_squared() <= 1.0f) {
      // if we're "at" this Node, increment the _curr_goal_node and try again
      _curr_goal_node++;
      continue;
    }

    // otherwise, move towards the goal...
    _moveable->set_intermediate_goal(goal);
    break;
  }
}

bool pathing_component::is_following_path() const {
  return (_path.size() > _curr_goal_node);
}

void pathing_component::set_path(std::vector<fw::Vector> const &path) {
  _path = path;
  _curr_goal_node = 0;

  fw::debug << "found path to [" << _last_request_goal << "]: " << _path.size() << " node(s)" << std::endl;
}

void pathing_component::set_goal(fw::Vector const &goal) {
  // request a path from the entity's current position to the given goal and get the pathing_thread to call our
  // set_path method when it's done
  float now = fw::Framework::get_instance()->get_timer()->get_total_time();
  if ((goal - _last_request_goal).length() < 1.0f) {
    // it's the same path as we're already following, don't request a new path more than one every few seconds
    // since there's really no point
    if (now - _last_request_time < 5.0f) {
      return;
    }
  }
  _last_request_time = now;
  _last_request_goal = goal;

  auto pathing_thread = game::world::get_instance()->get_pathing();
  pathing_thread->request_path(position_->get_position(), goal,
      std::bind(&pathing_component::on_path_found, this, _1));
}

void pathing_component::stop() {
  _path.clear();
  _curr_goal_node = 0;
}

void pathing_component::on_path_found(std::vector<fw::Vector> const &path) {
  set_path(path);
}

}
