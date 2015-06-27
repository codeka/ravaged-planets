#include <boost/foreach.hpp>

#include <framework/misc.h>
#include <framework/colour.h>
#include <game/entities/entity.h>
#include <game/entities/entity_manager.h>
#include <game/entities/entity_factory.h>
#include <game/entities/entity_debug.h>
#include <game/entities/moveable_component.h>
#include <game/entities/position_component.h>
#include <game/entities/selectable_component.h>

namespace ent {

// register the moveable component with the entity_factory
ENT_COMPONENT_REGISTER("moveable", moveable_component);

moveable_component::moveable_component() :
    _pos(0), _speed(3.0f), _turn_speed(1.0f), _avoid_collisions(true) {
}

moveable_component::~moveable_component() {
}

void moveable_component::apply_template(std::shared_ptr<entity_component_template> comp_template) {
  BOOST_FOREACH(auto &kvp, comp_template->properties) {
    if (kvp.first == "Speed") {
      _speed = boost::lexical_cast<float>(kvp.second);
    } else if (kvp.first == "TurnRadius") {
      _turn_speed = 1.0f / boost::lexical_cast<float>(kvp.second);
    } else if (kvp.first == "AvoidCollisions") {
      _avoid_collisions = boost::lexical_cast<bool>(kvp.second);
    }
  }

  entity_component::apply_template(comp_template);
}

void moveable_component::initialize() {
  std::shared_ptr<entity> entity(_entity);
  _pos = entity->get_component<position_component>();
  _goal = _pos->get_position();
}

void moveable_component::set_goal(fw::vector goal) {
  std::shared_ptr<entity> entity(_entity);
  float world_width = entity->get_manager()->get_patch_manager()->get_world_width();
  float world_length = entity->get_manager()->get_patch_manager()->get_world_length();

  // make sure we constraining the goal to the bounds of the map
  _goal = fw::vector(
      fw::constrain(goal[0], world_width, 0.0f),
      goal[1],
      fw::constrain(goal[2], world_length, 0.0f));
}

void moveable_component::update(float dt) {
  fw::vector pos = _pos->get_position();
  fw::vector goal = _goal;
  fw::vector dir = _pos->get_direction_to(goal);
  float distance = dir.length();
  if (distance < 0.1f) {
    // we're close enough to the goal, so just stop!
    return;
  }

  std::shared_ptr<entity> entity(_entity);
  bool show_steering = (entity->get_debug_view() != 0 && (entity->get_debug_flags() & debug_show_steering) != 0);

  // if we're avoiding obstacles, we'll need to figure out what is the closest entity to us
  if (_avoid_collisions) {
    std::shared_ptr<ent::entity> obstacle = _pos->get_nearest_entity().lock();
    if (obstacle) {
      fw::vector obstacle_dir = _pos->get_direction_to(obstacle);
      float obstacle_distance = obstacle_dir.length();

      // only worry about the obstacle when it's in front of us
      if (cml::dot(_pos->get_direction(), obstacle_dir) > 0.0f) {
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
          fw::vector obstacle_pos = pos + obstacle_dir;
          if (show_steering) {
            // draw a circle around whatever we're trying to avoid
            entity->get_debug_view()->add_circle(obstacle_pos, obstacle_radius * 2.0f, fw::colour(1, 0, 0));
          }

          // temporarily adjust the "goal" so as to avoid the obstacle
          fw::vector v = cml::cross(_pos->get_direction(), obstacle_dir);
          if (v.length_squared() < 0.01f) {
            // if they're *directly* in front of us, just choose a random direction, left
            // or right. we'll choose . . . left
            v = fw::vector(0, 1, 0);
          }
          fw::vector avoid_dir = cml::cross(_pos->get_direction(), v).normalize();

          if (show_steering) {
            // draw a blue line from the obstacle in the direction we're going to travel
            // to avoid it.
            entity->get_debug_view()->add_line(
                obstacle_pos, obstacle_pos + (avoid_dir * (obstacle_radius * 2.0f)), fw::colour(0, 0, 1));
          }

          goal = obstacle_pos + (avoid_dir * (obstacle_radius * 2.0f));
          dir = _pos->get_direction_to(goal);
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
    entity->get_debug_view()->add_line(pos, goal, fw::colour(0, 1, 0));
  }

  if (scale_factor < 1.0f) {
    speed *= (scale_factor < 0.75f ? 0.75f : scale_factor);
    turn_speed *= 1.0f / (scale_factor < 0.25f ? 0.25f : scale_factor);
  }

  // turn towards the goal
  dir = steer(_pos->get_position(), _pos->get_direction(), dir, turn_speed * dt, show_steering);

  // move in the direction we're facing
  pos += (dir * dt * speed);

  _pos->set_direction(dir);
  _pos->set_position(pos);
}

// applies a steering factor to the "curr_direction" so that we slowly turn towards
// the goal_direction.
//
// if show_steering is true, we use the entity's debug_view (which we assume is non-NULL
//   to display the relevent steering vectors.
fw::vector moveable_component::steer(fw::vector pos, fw::vector curr_direction, fw::vector goal_direction,
    float turn_amount, bool show_steering) {
  curr_direction = curr_direction.normalize();
  goal_direction = goal_direction.normalize();
  fw::vector up = cml::cross(curr_direction, goal_direction);

  // so, to work out the steering factor, we start off by rotating the direction
  // vector clockwise 90 degrees...
  fw::matrix rotation = fw::rotate_axis_angle(up, static_cast<float>(M_PI / 2.0));
  fw::vector steer = cml::transform_vector(rotation, curr_direction).normalize();

  // now, if the angle between steer and the goal is greater than 90,
  // we need to steer in the other direction.
  steer = cml::dot(steer, goal_direction) < 0.0f ? steer * -1.0f : steer;

  if (show_steering) {
    std::shared_ptr<entity> entity(_entity);
    entity_debug_view *edv = entity->get_debug_view();

    // draw the current "up" and "forward" vectors
    edv->add_line(pos, pos + up, fw::colour(1, 1, 1));
    edv->add_line(pos, pos + curr_direction, fw::colour(1, 1, 1));

    // draw the "steering" vector which is the direction we're going to steer in
    entity->get_debug_view()->add_line(pos, pos + steer, fw::colour(0, 1, 1));
  }

  // adjust the amount of steering by the turn_amount. The higher the turn amount, the
  // more we try to steer (we assume turn_amount is already scaled by the time delta
  // for this frame)
  steer *= turn_amount;

  // adjust our current heading by applying a steering factor
  fw::vector new_direction = (curr_direction + steer);
  new_direction.normalize();

  // if the new direction is on the other side of the goal_direction to the current
  // direction, that means we'd be over-steering. rather than do that (and wobble), we'll
  // just start heading straight for the goal.
  fw::vector old_direction = curr_direction;
  fw::vector rotated = cml::transform_vector(rotation, goal_direction).normalize();

  float old_direction_dot = cml::dot(old_direction, rotated);
  float new_direction_dot = cml::dot(new_direction, rotated);
  if ((old_direction_dot < 0 && new_direction_dot > 0) || (old_direction_dot > 0 && new_direction_dot < 0)) {
    return goal_direction;
  } else {
    return new_direction;
  }
}

}
