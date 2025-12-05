
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/math.h>
#include <framework/xml.h>
#include <framework/logging.h>
#include <framework/timer.h>

#include <game/entities/entity.h>
#include <game/entities/entity_debug.h>
#include <game/entities/entity_factory.h>
#include <game/entities/position_component.h>
#include <game/entities/moveable_component.h>

namespace ent {

Entity::Entity(EntityManager *mgr, entity_id id)
  : mgr_(mgr), debug_flags_(static_cast<EntityDebugFlags>(0)), id_(id), create_time_(0) {
}

Entity::~Entity() {
  for(auto& pair : components_) {
    delete pair.second;
  }
  for (auto& cleanup_fn : cleanup_functions_) {
    cleanup_fn();
  }
}

void Entity::add_component(EntityComponent *comp) {
  // you can only have one component of each type
  auto it = components_.find(comp->get_identifier());
  if (it != components_.end()) {
    LOG(ERR) << "only one component of each type is allowed: " << comp->get_identifier();
    return;
  }

  components_[comp->get_identifier()] = comp;
}

EntityComponent *Entity::get_component(int identifier) {
  auto it = components_.find(identifier);
  if (it == components_.end()) {
    return nullptr;
  }

  return (*it).second;
}

bool Entity::contains_component(int identifier) const {
  return (components_.find(identifier) != components_.end());
}

void Entity::add_attribute(EntityAttribute const &attr) {
  // you can only have one attribute with a given name
  auto it = attributes_.find(attr.get_name());
  if (it != attributes_.end()) {
    LOG(ERR) << "only one attribute with the same name is allowed: " << attr.get_name();
    return;
  }

  attributes_[attr.get_name()] = attr;
}

EntityAttribute *Entity::get_attribute(std::string const &name) {
  auto it = attributes_.find(name);
  if (it == attributes_.end())
    return nullptr;

  return &(*it).second;
}

void Entity::initialize() {
  create_time_ = fw::Framework::get_instance()->get_timer()->get_total_time();
  for(auto &pair : components_) {
    pair.second->initialize();
  }
}

void Entity::update(float dt) {
  for (auto& pair : components_) {
    pair.second->update(dt);
  }

  if (debug_view_ && debug_flags_ == 0) {
    debug_view_.reset();
  } else if (!debug_view_ && debug_flags_ != 0) {
    debug_view_ = std::make_unique<EntityDebugView>(this);
  }
  if (debug_view_) {
    debug_view_->update(dt);
  }
}

void Entity::set_position(fw::Vector const &pos) {
  PositionComponent *position = get_component<ent::PositionComponent>();
  if (position != nullptr) {
    position->set_position(pos);

    MoveableComponent *moveable = get_component<ent::MoveableComponent>();
    if (moveable != nullptr) {
      moveable->set_goal(position->get_position());
    }
  }
}

float Entity::get_age() const {
  float curr_time = fw::Framework::get_instance()->get_timer()->get_total_time();
  return (curr_time - create_time_);
}

//-------------------------------------------------------------------------

EntityComponent::EntityComponent() {
}

EntityComponent::~EntityComponent() {
}

}
