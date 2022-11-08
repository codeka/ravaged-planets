#pragma once

#include <memory>

#include <framework/math.h>

#include <game/entities/orderable_component.h>

namespace fw {
namespace net {
class PacketBuffer;
}
}

namespace ent {
class BuilderComponent;
class MoveableComponent;
}

namespace game {

/** Base class for our order classes. */
class Order {
private:
  std::string state_name_;

protected:
  std::weak_ptr<ent::Entity> entity_;

  Order(std::string const &state_name);

public:
  Order();
  Order(const Order&) = delete;
  virtual ~Order();

  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

  /**
   * This is called by the orderable_component when this order hits the front of the queue for Entity. You can
   * start executing the order now.
   */
  virtual void begin(std::weak_ptr<ent::Entity> const &ent);

  /** This is called each frame and lets you update your order status. */
  virtual void update(float) {
  }

  /**
   * This is called by the orderable_component at intervals, to check whether you've completed the order and it
   * should move onto the next one.
   */
  virtual bool is_complete() = 0;

  /**
   * Gets the name of the state the Entity is in while it's executing this order (e.g. "moving", "attacking",
   * "building", etc)
   */
  virtual std::string get_state_name() const {
    return state_name_;
  }

  virtual uint16_t get_identifier() const = 0;
};

/** This is the "build" order which you issue to unit that can build other units (such as the factory, etc). */
class BuildOrder: public Order {
private:
  ent::BuilderComponent *builder_;

public:
  BuildOrder();
  virtual ~BuildOrder();

  void begin(std::weak_ptr<ent::Entity> const &ent) override;
  bool is_complete() override;

  void serialize(fw::net::PacketBuffer &buffer) override;
  void deserialize(fw::net::PacketBuffer &buffer) override;

  std::string template_name;

  static const int identifier = 1;
  uint16_t get_identifier() const override {
    return identifier;
  }
};

/** This is the "move" order which you issue to a unit when you want it to move from point "A" to point "B". */
class MoveOrder: public Order {
public:
  MoveOrder();
  ~MoveOrder();

  void begin(std::weak_ptr<ent::Entity> const &ent) override;
  bool is_complete() override;

  void serialize(fw::net::PacketBuffer &buffer) override;
  void deserialize(fw::net::PacketBuffer &buffer) override;

  fw::Vector goal;

  static const int identifier = 2;
  uint16_t get_identifier() const override {
    return identifier;
  }
};

/** This is the "attack" order, which you issue to a unit when you want it to attack another unit. */
class AttackOrder: public Order {
private:
  void attack(std::shared_ptr<ent::Entity> Entity, std::shared_ptr<ent::Entity> target_entity);
public:
  AttackOrder();
  ~AttackOrder();

  void begin(std::weak_ptr<ent::Entity> const &ent) override;
  bool is_complete() override;

  void serialize(fw::net::PacketBuffer &buffer) override;
  void deserialize(fw::net::PacketBuffer &buffer) override;

  // ID of the Entity we want to attack.
  ent::entity_id target;

  static const int identifier = 3;
  uint16_t get_identifier() const override {
    return identifier;
  }
};

// creates the order object from the given identifier
std::shared_ptr<Order> create_order(uint16_t id);

template<typename T>
inline std::shared_ptr<T> create_order() {
  return std::dynamic_pointer_cast<T>(create_order(T::identifier));
}

}
