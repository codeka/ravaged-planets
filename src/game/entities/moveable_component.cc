#include <boost/foreach.hpp>

#include <framework/misc.h>
#include <framework/color.h>
#include <framework/logging.h>

#include <game/entities/entity.h>
#include <game/entities/entity_manager.h>
#include <game/entities/entity_factory.h>
#include <game/entities/entity_debug.h>
#include <game/entities/moveable_component.h>
#include <game/entities/pathing_component.h>
#include <game/entities/position_component.h>
#include <game/entities/selectable_component.h>

namespace ent {

// register the moveable component with the entity_factory
ENT_COMPONENT_REGISTER("Moveable", moveable_component);

moveable_component::moveable_component() :
    _position_component(nullptr), _pathing_component(nullptr), _speed(3.0f), _turn_speed(1.0f),
    _avoid_collisions(true), _is_moving(false) {
}

moveable_component::~moveable_component() {
}

void moveable_component::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "Speed") {
//      _speed = luabind::object_cast<float>(*it);
//    } else if (it.key() == "TurnRadius") {
//      _turn_speed = 1.0f / luabind::object_cast<float>(*it);
//    } else if (it.key() == "AvoidCollisions") {
//      _avoid_collisions = luabind::object_cast<bool>(*it);
//    }
//  }
}

void moveable_component::initialize() {
  std::shared_ptr<entity> entity(_entity);
  _pathing_component = entity->get_component<pathing_component>();
  _position_component = entity->get_component<position_component>();
  _goal = _position_component->get_position();
  _is_moving = false;
}

// TODO: skip_pathing is such a hack
void moveable_component::set_goal(fw::Vector goal, bool skip_pathing /*= false*/) {
  std::shared_ptr<entity> entity(_entity);
  float world_width = entity->get_manager()->get_patch_manager()->get_world_width();
  float world_length = entity->get_manager()->get_patch_manager()->get_world_length();

  // make sure we constraining the goal to the bounds of the map
  _goal = fw::Vector(
      fw::constrain(goal[0], world_width, 0.0f),
      goal[1],
      fw::constrain(goal[2], world_length, 0.0f));
  if (skip_pathing || _pathing_component == nullptr) {
    set_intermediate_goal(_goal);
  } else {
    _pathing_component->set_goal(_goal);
  }
}

void moveable_component::set_intermediate_goal(fw::Vector goal) {
  std::shared_ptr<entity> entity(_entity);
  float world_width = entity->get_manager()->get_patch_manager()->get_world_width();
  float world_length = entity->get_manager()->get_patch_manager()->get_world_length();

  // make sure we constraining the goal to the bounds of the map
  _intermediate_goal = fw::Vector(
      fw::constrain(goal[0], world_width, 0.0f),
      goal[1],
      fw::constrain(goal[2], world_length, 0.0f));
  _is_moving = true;
}

void moveable_component::stop() {
  _is_moving = false;
  if (_pathing_component != nullptr) {
    _pathing_component->stop();
  }
}

void moveable_component::update(float dt) {
  if (!_is_moving) {
    return;
  }

  fw::Vector pos = _position_component->get_position();
  fw::Vector goal = _intermediate_goal;
  fw::Vector dir = _position_component->get_direction_to(goal);
  float distance = dir.length();
  if (distance < 0.1f) {
    // we're close enough to the goal, so just stop!
    _is_moving = false;
    return;
  }

  std::shared_ptr<entity> entity(_entity);
  bool show_steering = (entity->get_debug_view() != 0 && (entity->get_debug_flags() & debug_show_steering) != 0);

  // if we're avoiding obstacles, we'll need to figure out what is the closest entity to us
  if (_avoid_collisions) {
    std::shared_ptr<ent::entity> obstacle = _position_component->get_nearest_entity().lock();
    if (obstacle) {
      fw::Vector obstacle_dir = _position_component->get_direction_to(obstacle);
      float obstacle_distance = obstacle_dir.length();

      // only worry about the obstacle when it's in front of us
      float d = cml::dot(cml::normalize(_position_component->get_direction()), cml::normalize(obstacle_dir));
    //  fw::debug << "cml::dot(_position_component->get_direction(), obstacle_dir) = " << d << std::endl;
      if (d > 0.0f) {
        // if they're selectable, reduce the distance by their selection radius - that's what we ACTUALLY want to
        // avoid...
        float obstacle_radius = 1.0f;
        selectable_component *their_selectable = obstacle->get_component<selectable_component>();
        if (their_selectable != nullptr) {
          obstacle_radius = their_selectable->get_selection_radius();
          obstacle_distance -= obstacle_radius;
        }

        // only worry about the obstacle if we're closer to it than to the goal...
        if (obstacle_distance < (obstacle_radius * 2.0f) && obstacle_distance < distance) {
          fw::Vector obstacle_pos = pos + obstacle_dir;
          if (show_steering) {
            // draw a circle around whatever we're trying to avoid
            entity->get_debug_view()->add_circle(obstacle_pos, obstacle_radius * 2.0f, fw::Color(1, 0, 0));
          }

          // temporarily adjust the "goal" so as to avoid the obstacle
          fw::Vector up = cml::cross(_position_component->get_direction(), obstacle_dir);
          if (up.length_squared() < 0.01f) {
            // if they're *directly* in front of us, just choose a Random direction, left or right. we'll choose... left
            up = fw::Vector(0, 1, 0);
          }
          fw::Vector avoid_dir = cml::cross(cml::normalize(_position_component->get_direction()), cml::normalize(up));

          if (show_steering) {
            // draw a blue line from the obstacle in the direction we're going to travel to avoid it.
            entity->get_debug_view()->add_line(
                obstacle_pos, obstacle_pos + (avoid_dir * (obstacle_radius * 2.0f)), fw::Color(0, 0, 1));
          }

          // our new goal is just in front of where we are now, but offset by what we're trying to avoid.
          goal = _position_component->get_direction() + obstacle_pos + (avoid_dir * (obstacle_radius * 2.0f));
          dir = _position_component->get_direction_to(goal);
          distance = dir.length();
        }
      }
    }
  }

  float speed = _speed;
  float turn_speed = _turn_speed;

  // adjust the speed and turn speed so that we slow down and turn faster when we get close
  // (gives us a smaller turning circle, to ensure we don't overshoot the target)
  float turn_radius = 1.0f / _turn_speed;
  float scale_factor = distance / (turn_radius / 0.25f);

  if (show_steering) {
    // a line from us to the goal
    entity->get_debug_view()->add_line(pos, goal, fw::Color(0, 1, 0));
  }

  if (scale_factor < 1.0f) {
    speed *= (scale_factor < 0.75f ? 0.75f : scale_factor);
    turn_speed *= 1.0f / (scale_factor < 0.25f ? 0.25f : scale_factor);
  }

  // turn towards the goal
  dir = steer(_position_component->get_position(),
      _position_component->get_direction(), dir, turn_speed * dt, show_steering);

  // move in the direction we're facing
  pos += (dir * dt * speed);

  _position_component->set_direction(dir);
  _position_component->set_position(pos);
}

// applies a steering factor to the "curr_direction" so that we slowly turn towards the goal_direction.
//
// if show_steering is true, we use the entity's debug_view (which we assume is non-NULL
//   to display the relevent steering vectors.
fw::Vector moveable_component::steer(fw::Vector pos, fw::Vector curr_direction, fw::Vector goal_direction,
    float turn_amount, bool show_steering) {
  curr_direction.normalize();
  goal_direction.normalize();

  // If we're almost facing the right direction already, just point straight towards the goal, no steering.
  if (cml::dot(curr_direction, goal_direction) > 0.95f) {
    return goal_direction;
  }

  // so, to work out the steering factor, we start off by rotating the direction vector clockwise 90 degrees...
  fw::Vector up = cml::cross(curr_direction, goal_direction);
  fw::Matrix rotation = fw::rotate_axis_angle(up, static_cast<float>(M_PI / 2.0));
  fw::Vector steer = cml::transform_vector(rotation, curr_direction).normalize();

  // now, if the angle between steer and the goal is greater than 90,
  // we need to steer in the other direction.
  steer = cml::dot(steer, goal_direction) < 0.0f ? steer * -1.0f : steer;

  if (show_steering) {
    std::shared_ptr<entity> entity(_entity);
    entity_debug_view *edv = entity->get_debug_view();

    // draw the current "up" and "forward" vectors
    edv->add_line(pos, pos + up, fw::Color(1, 1, 1));
    edv->add_line(pos, pos + curr_direction, fw::Color(1, 1, 1));

    // draw the "steering" vector which is the direction we're going to steer in
    entity->get_debug_view()->add_line(pos, pos + steer, fw::Color(0, 1, 1));
  }

  // adjust the amount of steering by the turn_amount. The higher the turn amount, the more we try to steer (we assume
  // turn_amount is already scaled by the time delta for this frame)
  steer *= turn_amount;

  // adjust our current heading by applying a steering factor
  fw::Vector new_direction = (curr_direction + steer);
  new_direction.normalize();

  // if the new direction is on the other side of the goal_direction to the current
  // direction, that means we'd be over-steering. rather than do that (and wobble), we'll
  // just start heading straight for the goal.
  fw::Vector old_direction = curr_direction;
  fw::Vector rotated = cml::transform_vector(rotation, goal_direction).normalize();

  float old_direction_dot = cml::dot(old_direction, rotated);
  float new_direction_dot = cml::dot(new_direction, rotated);
  if ((old_direction_dot < 0 && new_direction_dot > 0) || (old_direction_dot > 0 && new_direction_dot < 0)) {
    return goal_direction;
  } else {
    return new_direction;
  }
}

}
