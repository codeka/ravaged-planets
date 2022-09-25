#include <functional>
#include <memory>

#include <framework/packet_buffer.h>
#include <framework/exception.h>

#include <game/simulation/orders.h>
#include <game/entities/builder_component.h>
#include <game/entities/entity_manager.h>
#include <game/entities/orderable_component.h>
#include <game/entities/moveable_component.h>
#include <game/entities/position_component.h>
#include <game/entities/weapon_component.h>
#include <game/entities/pathing_component.h>
#include <game/world/world.h>

namespace game {

//-------------------------------------------------------------------------
// this is a helper macro for registering order types with the order_factory
#define ORDER_REGISTER(type) \
  std::shared_ptr<order> create_ ## type () { return std::shared_ptr<order>(new type()); } \
  order_registrar reg_ ## type(type::identifier, &create_ ## type)

typedef std::function<std::shared_ptr<order>()> create_order_fn;
typedef std::map<uint16_t, create_order_fn> order_registry_map;
static order_registry_map *g_order_registry = nullptr;

// this is a helper class that you use indirectly via the ORDER_REGISTER macro
// to register an order.
class order_registrar {
public:
  order_registrar(uint16_t id, create_order_fn fn);
};

order_registrar::order_registrar(uint16_t id, create_order_fn fn) {
  if (g_order_registry == 0)
    g_order_registry = new order_registry_map();

  (*g_order_registry)[id] = fn;
}

// creates the order object from the given order identifier
std::shared_ptr<order> create_order(uint16_t id) {
  create_order_fn fn = (*g_order_registry)[id];
  if (fn == nullptr) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("order does not exist: " + boost::lexical_cast<std::string>(id)));
  }

  return fn();
}

//-------------------------------------------------------------------------
ORDER_REGISTER(build_order);
ORDER_REGISTER(move_order);
ORDER_REGISTER(attack_order);

//-------------------------------------------------------------------------

order::order(std::string const &state_name) :
    _state_name(state_name) {
}

order::~order() {
}

void order::begin(std::weak_ptr<ent::Entity> const &ent) {
  entity_ = ent;
}

void order::serialize(fw::net::PacketBuffer &) {
}

void order::deserialize(fw::net::PacketBuffer &) {
}

//-------------------------------------------------------------------------

build_order::build_order() :
    order("building"), _builder(0) {
}

build_order::~build_order() {
}

void build_order::begin(std::weak_ptr<ent::Entity> const &ent) {
  order::begin(ent);

  std::shared_ptr<ent::Entity> Entity(entity_);
  _builder = Entity->get_component<ent::BuilderComponent>();
  if (_builder != nullptr) {
    _builder->build(template_name);
  }
}

bool build_order::is_complete() {
  if (_builder == 0)
    return true;

  return !_builder->is_building();
}

void build_order::serialize(fw::net::PacketBuffer &buffer) {
  buffer << template_name;
}

void build_order::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> template_name;
}

//-------------------------------------------------------------------------

move_order::move_order() :
    order("moving") {
}

move_order::~move_order() {
}

void move_order::begin(std::weak_ptr<ent::Entity> const &ent) {
  order::begin(ent);
  std::shared_ptr<ent::Entity> Entity(entity_);

  // move towards the component, if we don't have a moveable component, nothing will happen.
  auto moveable = Entity->get_component<ent::MoveableComponent>();
  if (moveable != nullptr) {
    moveable->set_goal(goal);
  }

  // if it's got a weapon, clear the target (since we've now moving instead)
  ent::WeaponComponent *weapon = Entity->get_component<ent::WeaponComponent>();
  if (weapon != 0) {
    weapon->clear_target();
  }
}

bool move_order::is_complete() {
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (Entity) {
    ent::PathingComponent *pathing = Entity->get_component<ent::PathingComponent>();
    if (pathing != nullptr) {
      return !pathing->is_following_path();
    }

    ent::PositionComponent *pos = Entity->get_component<ent::PositionComponent>();
    return (pos->get_direction_to(goal).length_squared() < 1.1f);
  }

  return true;
}

void move_order::serialize(fw::net::PacketBuffer &buffer) {
  buffer << goal;
}

void move_order::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> goal;
}

//-----------------------------------------------------------------------------

attack_order::attack_order() :
    order("attacking") {
}

attack_order::~attack_order() {
}

void attack_order::begin(std::weak_ptr<ent::Entity> const &ent) {
  order::begin(ent);
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (Entity) {
    std::weak_ptr<ent::Entity> target_entity_wp = game::world::get_instance()->get_entity_manager()->get_entity(target);
    std::shared_ptr<ent::Entity> target_entity = target_entity_wp.lock();
    if (target_entity) {
      attack(Entity, target_entity);
    }
  }
}

void attack_order::attack(std::shared_ptr<ent::Entity> Entity, std::shared_ptr<ent::Entity> target_entity) {
  ent::WeaponComponent *weapon = Entity->get_component<ent::WeaponComponent>();
  if (weapon != nullptr) {
    weapon->set_target(target_entity);
  }
}

bool attack_order::is_complete() {
  // attack is complete when either of us is dead
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (!Entity) {
    return true;
  }

  std::weak_ptr<ent::Entity> target_entity = game::world::get_instance()->get_entity_manager()->get_entity(target);
  if (!target_entity.lock()) {
    return true;
  }

  return false;
}

void attack_order::serialize(fw::net::PacketBuffer &buffer) {
  buffer << target;
}

void attack_order::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> target;
}


}
