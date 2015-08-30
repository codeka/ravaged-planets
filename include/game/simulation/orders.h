#pragma once

#include <memory>
#include <framework/vector.h>
#include <game/entities/orderable_component.h>

namespace fw {
namespace net {
class packet_buffer;
}
}

namespace ent {
class builder_component;
class moveable_component;
}

namespace game {

/** Base class for our order classes. */
class order: boost::noncopyable {
private:
  std::string _state_name;

protected:
  std::weak_ptr<ent::entity> _entity;

  order(std::string const &state_name);

public:
  virtual ~order();

  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

  /**
   * This is called by the orderable_component when this order hits the front of the queue for entity. You can
   * start executing the order now.
   */
  virtual void begin(std::weak_ptr<ent::entity> const &ent);

  /** This is called each frame and lets you update your order status. */
  virtual void update(float) {
  }

  /**
   * This is called by the orderable_component at intervals, to check whether you've completed the order and it
   * should move onto the next one.
   */
  virtual bool is_complete() = 0;

  /**
   * Gets the name of the state the entity is in while it's executing this order (e.g. "moving", "attacking",
   * "building", etc)
   */
  virtual std::string get_state_name() const {
    return _state_name;
  }

  virtual uint16_t get_identifier() const = 0;
};

/** This is the "build" order which you issue to unit that can build other units (such as the factory, etc). */
class build_order: public order {
private:
  ent::builder_component *_builder;

public:
  build_order();
  virtual ~build_order();

  virtual void begin(std::weak_ptr<ent::entity> const &ent);
  virtual bool is_complete();

  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

  std::string template_name;

  static const int identifier = 1;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

/** This is the "move" order which you issue to a unit when you want it to move from point "A" to point "B". */
class move_order: public order {
public:
  move_order();
  ~move_order();

  virtual void begin(std::weak_ptr<ent::entity> const &ent);
  virtual bool is_complete();

  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

  fw::vector goal;

  static const int identifier = 2;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

/** This is the "attack" order, which you issue to a unit when you want it to attack another unit. */
class attack_order: public order {
private:
  void attack(std::shared_ptr<ent::entity> entity, std::shared_ptr<ent::entity> target_entity);
public:
  attack_order();
  ~attack_order();

  virtual void begin(std::weak_ptr<ent::entity> const &ent);
  virtual bool is_complete();

  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

  // ID of the entity we want to attack.
  ent::entity_id target;

  static const int identifier = 3;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// creates the order object from the given identifier
std::shared_ptr<order> create_order(uint16_t id);

template<typename T>
inline std::shared_ptr<T> create_order() {
  return std::dynamic_pointer_cast<T>(create_order(T::identifier));
}

}
