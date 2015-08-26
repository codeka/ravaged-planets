#include <functional>
#include <framework/logging.h>

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
    _position(nullptr), _moveable(nullptr), _curr_goal_node(0) {
}

pathing_component::~pathing_component() {
}

void pathing_component::initialize() {
  std::shared_ptr<entity> ent = _entity.lock();
  if (ent) {
    _position = ent->get_component<position_component>();
    _moveable = ent->get_component<moveable_component>();
  }
}

void pathing_component::update(float dt) {
  // follow the path... todo: this can be done SOOOOOO much better!
  while (is_following_path()) {
    fw::vector goal = _path[_curr_goal_node];
    fw::vector dir = _position->get_direction_to(goal);
    if (dir.length_squared() <= 1.0f) {
      // if we're "at" this node, increment the _curr_goal_node
      // and try again
      _curr_goal_node++;
      continue;
    }

    // otherwise, move towards the goal...
    _moveable->set_goal(goal);
    break;
  }
}

bool pathing_component::is_following_path() const {
  return (_path.size() > _curr_goal_node);
}

void pathing_component::set_path(std::vector<fw::vector> const &path) {
  _path = path;
  _curr_goal_node = 0;

  fw::debug << "found path: " << _path.size() << " node(s)" << std::endl;
}

void pathing_component::set_goal(fw::vector const &goal) {
  // request a path from the entity's current position to the given goal
  // and get the pathing_thread to call our set_path method when it's done
  fw::debug << "requesting new path: " << goal << std::endl;

  auto pathing_thread = game::world::get_instance()->get_pathing();
  pathing_thread->request_path(_position->get_position(), goal,
      std::bind(&pathing_component::on_path_found, this, _1));
}

void pathing_component::on_path_found(std::vector<fw::vector> const &path) {
  set_path(path);
}

}
