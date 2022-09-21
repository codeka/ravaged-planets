#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/vector.h>
#include <framework/xml.h>
#include <framework/logging.h>
#include <framework/timer.h>
#include <framework/exception.h>

#include <game/entities/entity.h>
#include <game/entities/entity_debug.h>
#include <game/entities/entity_factory.h>
#include <game/entities/position_component.h>
#include <game/entities/moveable_component.h>

namespace ent {

entity::entity(entity_manager *mgr, entity_id id) :
    mgr_(mgr), _debug_view(0), _debug_flags(static_cast<entity_debug_flags>(0)), id_(id),
    _create_time(0) {
}

entity::~entity() {
  BOOST_FOREACH(auto &pair, _components) {
    delete pair.second;
  }
}

void entity::add_component(entity_component *comp) {
  // you can only have one component of each type
  auto it = _components.find(comp->get_identifier());
  if (it != _components.end())
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("only one component of each type is allowed."));

  _components[comp->get_identifier()] = comp;
}

entity_component *entity::get_component(int identifier) {
  auto it = _components.find(identifier);
  if (it == _components.end()) {
    return nullptr;
  }

  return (*it).second;
}

bool entity::contains_component(int identifier) const {
  return (_components.find(identifier) != _components.end());
}

void entity::add_attribute(entity_attribute const &attr) {
  // you can only have one attribute with a given name
  auto it = _attributes.find(attr.get_name());
  if (it != _attributes.end()) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("only one attribute with the same name is allowed"));
  }

  _attributes[attr.get_name()] = attr;
}

entity_attribute *entity::get_attribute(std::string const &name) {
  auto it = _attributes.find(name);
  if (it == _attributes.end())
    return nullptr;

  return &(*it).second;
}

void entity::initialize() {
  _create_time = fw::framework::get_instance()->get_timer()->get_total_time();
  BOOST_FOREACH(auto &pair, _components) {
    pair.second->initialize();
  }
}

void entity::update(float dt) {
  BOOST_FOREACH(auto &pair, _components) {
    pair.second->update(dt);
  }
}

void entity::render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &transform) {
  BOOST_FOREACH(auto &pair, _components) {
    pair.second->render(Scenegraph, transform);
  }

  if (_debug_view != nullptr) {
    _debug_view->render(Scenegraph, transform);
  }
}

void entity::set_position(fw::Vector const &pos) {
  position_component *position = get_component<ent::position_component>();
  if (position != nullptr) {
    position->set_position(pos);

    moveable_component *moveable = get_component<ent::moveable_component>();
    if (moveable != nullptr) {
      moveable->set_goal(position->get_position());
    }
  }
}

float entity::get_age() const {
  float curr_time = fw::framework::get_instance()->get_timer()->get_total_time();
  return (curr_time - _create_time);
}

// gets the entity_debug_view object that contains the list of lines and points that
// we'll draw along with this entity for debugging purposes.
entity_debug_view *entity::get_debug_view() {
  if (_debug_flags != 0) {
    if (_debug_view == nullptr) {
      _debug_view = new entity_debug_view();
    }

    return _debug_view;
  } else {
    delete _debug_view;
    return 0;
  }
}

//-------------------------------------------------------------------------

entity_component::entity_component() {
}

entity_component::~entity_component() {
}

}
