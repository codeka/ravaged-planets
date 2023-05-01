
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
ENT_COMPONENT_REGISTER("Moveable", MoveableComponent);

MoveableComponent::MoveableComponent() :
    position_component_(nullptr), pathing_component_(nullptr), speed_(3.0f), turn_speed_(1.0f),
    avoid_collisions_(true), is_moving_(false) {
}

MoveableComponent::~MoveableComponent() {
}

void MoveableComponent::apply_template(fw::lua::Value tmpl) {
  if (tmpl.has_key("Speed")) {
    speed_ = tmpl["Speed"];
  }
  if (tmpl.has_key("TurnRadius")) {
    turn_speed_ = 1.0f / static_cast<float>(tmpl["TurnRadius"]);
  }
  if (tmpl.has_key("AvoidCollisions")) {
    avoid_collisions_ = tmpl["AvoidCollisions"];
  }
}

void MoveableComponent::initialize() {
  std::shared_ptr<Entity> entity(entity_);
  pathing_component_ = entity->get_component<PathingComponent>();
  position_component_ = entity->get_component<PositionComponent>();
  goal_ = position_component_->get_position();
  is_moving_ = false;
}

// TODO: skip_pathing is such a hack
void MoveableComponent::set_goal(fw::Vector goal, bool skip_pathing /*= false*/) {
  std::shared_ptr<Entity> entity(entity_);
  float world_width = entity->get_manager()->get_patch_manager()->get_world_width();
  float world_length = entity->get_manager()->get_patch_manager()->get_world_length();

  // make sure we constraining the goal to the bounds of the map
  goal_ = fw::Vector(
      fw::constrain(goal[0], world_width, 0.0f),
      0.0f,
      fw::constrain(goal[2], world_length, 0.0f));
  if (skip_pathing || pathing_component_ == nullptr) {
    set_intermediate_goal(goal_);
  } else {
    // Start off with intermedia_goal set to the current position, and stop moving until we get a path.
    intermediate_goal_ = position_component_->get_position();
    is_moving_ = false;

    pathing_component_->set_goal(goal_);
  }
}

void MoveableComponent::set_intermediate_goal(fw::Vector goal) {
  std::shared_ptr<Entity> entity(entity_);
  float world_width = entity->get_manager()->get_patch_manager()->get_world_width();
  float world_length = entity->get_manager()->get_patch_manager()->get_world_length();

  // make sure we constraining the goal to the bounds of the map
  intermediate_goal_ = fw::Vector(
      fw::constrain(goal[0], world_width, 0.0f),
      0.0f,
      fw::constrain(goal[2], world_length, 0.0f));
  is_moving_ = true;
}

void MoveableComponent::stop() {
  is_moving_ = false;
  if (pathing_component_ != nullptr) {
    pathing_component_->stop();
  }
}

void MoveableComponent::update(float dt) {
  if (!is_moving_) {
    return;
  }

  fw::Vector pos = position_component_->get_position();
  pos[1] = 0.0f; // ignore terrain height.
  fw::Vector goal = intermediate_goal_;
  fw::Vector dir = position_component_->get_direction_to(goal, /*ignore_height=*/true);
  float distance = dir.length();
  if (distance < 0.1f) {
    // we're close enough to the goal, so just stop!
    is_moving_ = false;
    return;
  }

  std::shared_ptr<Entity> entity(entity_);
  const bool show_steering = (entity->has_debug_view() && (entity->get_debug_flags() & kDebugShowSteering) != 0);

  // if we're avoiding obstacles, we'll need to figure out what is the closest Entity to us
  if (avoid_collisions_) {
    std::shared_ptr<ent::Entity> obstacle = position_component_->get_nearest_entity().lock();
    if (obstacle) {
      fw::Vector obstacle_dir = position_component_->get_direction_to(obstacle);
      float obstacle_distance = obstacle_dir.length();

      // only worry about the obstacle when it's in front of us
      float d = fw::dot(position_component_->get_direction().normalized(), obstacle_dir.normalized());
    //  fw::debug << "cml::dot(_position_component->get_direction(), obstacle_dir) = " << d << std::endl;
      if (d > 0.0f) {
        // if they're selectable, reduce the distance by their selection radius - that's what we ACTUALLY want to
        // avoid...
        float obstacle_radius = 1.0f;
        SelectableComponent *their_selectable = obstacle->get_component<SelectableComponent>();
        if (their_selectable != nullptr) {
          obstacle_radius = their_selectable->get_selection_radius();
          obstacle_distance -= obstacle_radius;
        }

        // only worry about the obstacle if we're closer to it than to the goal...
        if (obstacle_distance < (obstacle_radius * 2.0f) && obstacle_distance < distance) {
          fw::Vector obstacle_pos = pos + obstacle_dir;
          if (show_steering) {
            // draw a circle around whatever we're trying to avoid
            entity->get_debug_view().add_circle(obstacle_pos, obstacle_radius * 2.0f, fw::Color(1, 0, 0));
          }

          // temporarily adjust the "goal" so as to avoid the obstacle
          fw::Vector up = fw::cross(position_component_->get_direction(), obstacle_dir);
          if (up.length() < 0.01f) {
            // if they're *directly* in front of us, just choose a random direction, left or right. we'll choose.. left
            up = fw::Vector(0, 1, 0);
          }
          fw::Vector avoid_dir = fw::cross(position_component_->get_direction().normalized(), up.normalized());

          if (show_steering) {
            // draw a blue line from the obstacle in the direction we're going to travel to avoid it.
            entity->get_debug_view().add_line(
                obstacle_pos, obstacle_pos + (avoid_dir * (obstacle_radius * 2.0f)), fw::Color(0, 0, 1));
          }

          // our new goal is just in front of where we are now, but offset by what we're trying to avoid.
          goal = position_component_->get_direction() + obstacle_pos + (avoid_dir * (obstacle_radius * 2.0f));
          dir = position_component_->get_direction_to(goal);
          distance = dir.length();
        }
      }
    }
  }

  float speed = speed_;
  float turn_speed = turn_speed_;

  // adjust the speed and turn speed so that we slow down and turn faster when we get close (gives us a smaller
  // turning circle, to ensure we don't overshoot the target)
  float turn_radius = 1.0f / turn_speed_;
  float scale_factor = distance / (turn_radius / 0.25f);

  if (show_steering) {
    // a line from us to the goal
    entity->get_debug_view().add_line(pos, goal, fw::Color(0, 1, 0), /*offset_terrain_height=*/true);
  }

  if (scale_factor < 1.0f) {
    speed *= (scale_factor < 0.75f ? 0.75f : scale_factor);
    turn_speed *= 1.0f / (scale_factor < 0.25f ? 0.25f : scale_factor);
  }

  // turn towards the goal
  dir = steer(position_component_->get_position(),
      position_component_->get_direction(), dir, turn_speed * dt, show_steering);

  // move in the direction we're facing
  pos += (dir * dt * speed);

  position_component_->set_direction(dir);
  position_component_->set_position(pos);
}

// applies a steering factor to the "curr_direction" so that we slowly turn towards the goal_direction.
//
// if show_steering is true, we use the Entity's debug_view (which we assume is non-NULL
//   to display the relevent steering vectors.
fw::Vector MoveableComponent::steer(fw::Vector pos, fw::Vector curr_direction, fw::Vector goal_direction,
    float turn_amount, bool show_steering) {
  curr_direction.normalize();
  goal_direction.normalize();

  // If we're almost facing the right direction already, just point straight towards the goal, no steering.
  if (fw::dot(curr_direction, goal_direction) > 0.95f) {
    return goal_direction;
  }

  // so, to work out the steering factor, we start off by rotating the direction vector clockwise 90 degrees...
  fw::Vector up = fw::cross(curr_direction, goal_direction).normalized();
  fw::Quaternion rotation = fw::rotate_axis_angle(up, fw::pi() / 2.0f);
  fw::Vector steer = (rotation * curr_direction).normalized();

  // now, if the angle between steer and the goal is greater than 90,
  // we need to steer in the other direction.
  steer = fw::dot(steer, goal_direction) < 0.0f ? steer * -1.0f : steer;

  if (show_steering) {
    auto entity = entity_.lock();
    if (entity) {
      // draw the current "up" and "forward" vectors
      entity->get_debug_view().add_line(pos, pos + up, fw::Color(1, 1, 1));
      entity->get_debug_view().add_line(pos, pos + curr_direction, fw::Color(1, 1, 1));

      // draw the "steering" vector which is the direction we're going to steer in
      entity->get_debug_view().add_line(pos, pos + steer, fw::Color(0, 1, 1));
    }
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
  fw::Vector rotated = (rotation * goal_direction).normalized();

  float old_direction_dot = fw::dot(old_direction, rotated);
  float new_direction_dot = fw::dot(new_direction, rotated);
  if ((old_direction_dot < 0 && new_direction_dot > 0) || (old_direction_dot > 0 && new_direction_dot < 0)) {
    return goal_direction;
  } else {
    return new_direction;
  }
}

}
