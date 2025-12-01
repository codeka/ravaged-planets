#include <functional>
#include <memory>

#include <absl/strings/str_cat.h>

#include <framework/packet_buffer.h>
#include <framework/status.h>

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
  std::shared_ptr<Order> create_ ## type () { return std::shared_ptr<Order>(new type()); } \
  OrderRegistrar reg_ ## type(type::identifier, &create_ ## type)

typedef std::function<std::shared_ptr<Order>()> create_order_fn;
typedef std::map<uint16_t, create_order_fn> order_registry_map;
static order_registry_map *g_order_registry = nullptr;

// this is a helper class that you use indirectly via the ORDER_REGISTER macro
// to register an order.
class OrderRegistrar {
public:
  OrderRegistrar(uint16_t id, create_order_fn fn);
};

OrderRegistrar::OrderRegistrar(uint16_t id, create_order_fn fn) {
  if (g_order_registry == 0)
    g_order_registry = new order_registry_map();

  (*g_order_registry)[id] = fn;
}

// creates the order object from the given order identifier
fw::StatusOr<std::shared_ptr<Order>> CreateOrder(uint16_t id) {
  create_order_fn fn = (*g_order_registry)[id];
  if (fn == nullptr) {
    return fw::ErrorStatus(absl::StrCat("order does not exist: ", id));
  }

  return fn();
}

//-------------------------------------------------------------------------
ORDER_REGISTER(BuildOrder);
ORDER_REGISTER(MoveOrder);
ORDER_REGISTER(AttackOrder);

//-------------------------------------------------------------------------

Order::Order(std::string const &state_name) :
    state_name_(state_name) {
}

Order::~Order() {
}

void Order::begin(std::weak_ptr<ent::Entity> const &ent) {
  entity_ = ent;
}

void Order::serialize(fw::net::PacketBuffer &) {
}

void Order::deserialize(fw::net::PacketBuffer &) {
}

//-------------------------------------------------------------------------

BuildOrder::BuildOrder() :
    Order("building"), builder_(0) {
}

BuildOrder::~BuildOrder() {
}

void BuildOrder::begin(std::weak_ptr<ent::Entity> const &ent) {
  Order::begin(ent);

  std::shared_ptr<ent::Entity> Entity(entity_);
  builder_ = Entity->get_component<ent::BuilderComponent>();
  if (builder_ != nullptr) {
    builder_->build(template_name);
  }
}

bool BuildOrder::is_complete() {
  if (builder_ == 0)
    return true;

  return !builder_->is_building();
}

void BuildOrder::serialize(fw::net::PacketBuffer &buffer) {
  buffer << template_name;
}

void BuildOrder::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> template_name;
}

//-------------------------------------------------------------------------

MoveOrder::MoveOrder() :
    Order("moving") {
}

MoveOrder::~MoveOrder() {
}

void MoveOrder::begin(std::weak_ptr<ent::Entity> const &ent) {
  Order::begin(ent);
  std::shared_ptr<ent::Entity> entity(entity_);

  // move towards the component, if we don't have a moveable component, nothing will happen.
  auto moveable = entity->get_component<ent::MoveableComponent>();
  if (moveable != nullptr) {
    moveable->set_goal(goal);
  }

  // if it has a builder component, then "move" commands actually just update the builder's target.
  auto builder = entity->get_component<ent::BuilderComponent>();
  if (builder != nullptr) {
    builder->set_target(goal);
  }

  // if it's got a weapon, clear the target (since we've now moving instead)
  ent::WeaponComponent *weapon = entity->get_component<ent::WeaponComponent>();
  if (weapon != 0) {
    weapon->clear_target();
  }
}

bool MoveOrder::is_complete() {
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (Entity) {
    ent::PathingComponent *pathing = Entity->get_component<ent::PathingComponent>();
    if (pathing != nullptr) {
      return !pathing->is_following_path();
    }

    ent::PositionComponent *pos = Entity->get_component<ent::PositionComponent>();
    return (pos->get_direction_to(goal).length() < 1.1f);
  }

  return true;
}

void MoveOrder::serialize(fw::net::PacketBuffer &buffer) {
  buffer << goal;
}

void MoveOrder::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> goal;
}

//-----------------------------------------------------------------------------

AttackOrder::AttackOrder() :
    Order("attacking") {
}

AttackOrder::~AttackOrder() {
}

void AttackOrder::begin(std::weak_ptr<ent::Entity> const &ent) {
  Order::begin(ent);
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (Entity) {
    std::weak_ptr<ent::Entity> target_entity_wp = game::World::get_instance()->get_entity_manager()->get_entity(target);
    std::shared_ptr<ent::Entity> target_entity = target_entity_wp.lock();
    if (target_entity) {
      attack(Entity, target_entity);
    }
  }
}

void AttackOrder::attack(std::shared_ptr<ent::Entity> Entity, std::shared_ptr<ent::Entity> target_entity) {
  ent::WeaponComponent *weapon = Entity->get_component<ent::WeaponComponent>();
  if (weapon != nullptr) {
    weapon->set_target(target_entity);
  }
}

bool AttackOrder::is_complete() {
  // attack is complete when either of us is dead
  std::shared_ptr<ent::Entity> Entity = entity_.lock();
  if (!Entity) {
    return true;
  }

  std::weak_ptr<ent::Entity> target_entity = game::World::get_instance()->get_entity_manager()->get_entity(target);
  if (!target_entity.lock()) {
    return true;
  }

  return false;
}

void AttackOrder::serialize(fw::net::PacketBuffer &buffer) {
  buffer << target;
}

void AttackOrder::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> target;
}


}
