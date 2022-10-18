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
ENT_COMPONENT_REGISTER("Pathing", PathingComponent);

PathingComponent::PathingComponent() :
    position_(nullptr), moveable_(nullptr), curr_goal_node_(0), last_request_time_(0.0f) {
}

PathingComponent::~PathingComponent() {
}

void PathingComponent::initialize() {
  std::shared_ptr<Entity> ent = entity_.lock();
  if (ent) {
    position_ = ent->get_component<PositionComponent>();
    moveable_ = ent->get_component<MoveableComponent>();
  }
}

void PathingComponent::update(float dt) {
  auto entity = entity_.lock();
  if (!entity) return;

  // follow the path... todo: this can be done SOOOOOO much better!
  while (is_following_path()) {
    fw::Vector goal = path_[curr_goal_node_];
    fw::Vector dir = position_->get_direction_to(goal);
    dir[1] = 0.0f; // ignore height component
    if (dir.length_squared() <= 1.0f) {
      // if we're "at" this Node, increment the _curr_goal_node and try again
      curr_goal_node_++;
      continue;
    }

    // otherwise, move towards the goal...
    moveable_->set_intermediate_goal(goal);
    break;
  }

  if (entity->has_debug_view() && (entity->get_debug_flags() & EntityDebugFlags::kDebugShowPathing) != 0) {
    for (int i = 0; i < static_cast<int>(path_.size()) - 1; i++) {
      const fw::Vector& from = path_[i];
      const fw::Vector& to = path_[i + 1];
      entity->get_debug_view().add_line(from, to, fw::Color(0, 1, 1));
    }
  }
}

bool PathingComponent::is_following_path() const {
  return (path_.size() > curr_goal_node_);
}

void PathingComponent::set_path(std::vector<fw::Vector> const &path) {
  path_ = path;
  curr_goal_node_ = 0;

  fw::debug << "found path to [" << last_request_goal_ << "]: " << path_.size() << " node(s)" << std::endl;
}

void PathingComponent::set_goal(fw::Vector const &goal) {
  // request a path from the Entity's current position to the given goal and get the pathing_thread to call our
  // set_path method when it's done
  float now = fw::Framework::get_instance()->get_timer()->get_total_time();
  if ((goal - last_request_goal_).length() < 1.0f) {
    // it's the same path as we're already following, don't request a new path more than one every few seconds
    // since there's really no point
    if (now - last_request_time_ < 5.0f) {
      return;
    }
  }
  last_request_time_ = now;
  last_request_goal_ = goal;

  auto pathing_thread = game::World::get_instance()->get_pathing();
  pathing_thread->request_path(position_->get_position(), goal,
      std::bind(&PathingComponent::on_path_found, this, _1));
}

void PathingComponent::stop() {
  path_.clear();
  curr_goal_node_ = 0;
}

void PathingComponent::on_path_found(std::vector<fw::Vector> const &path) {
  set_path(path);
}

}
