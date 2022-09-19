#pragma once

#include <memory>
#include <framework/vector.h>
#include <game/simulation/simulation_thread.h>
#include <game/entities/entity.h>

namespace fw {
namespace net {
class packet_buffer;
}
}

namespace game {
class player;
class order;

/**
 * The command is the base class for what the simulation_thread processes. Each turn, we process the commands that
 * have been queued up for that turn. Each "action" in the game is represented by a command.
 */
class command {
private:
  player *_player;

protected:
  command(uint8_t player_no);

public:
  virtual ~command();

  virtual void serialize(fw::net::packet_buffer &buffer) = 0;
  virtual void deserialize(fw::net::packet_buffer &buffer) = 0;

  /** Gets an instance of the player who executed this command. */
  player *get_player() const {
    return _player;
  }

  /** When it's our turn, this'll get called to actually execute the command. */
  virtual void execute() = 0;

  /**
   * Gets the identifier of this command, which is used to differentiate between all the commands
   * in the game (e.g. move vs. attack vs. build, etc)
   */
  virtual uint8_t get_identifier() const = 0;
};

/**
 * This command is sent to another player when we want to notify them about a new player (i.e. when we first connect
 * to them [as a non-host], or when we add a new AI player)
 */
class connect_player_command: public command {
private:

public:
  connect_player_command(uint8_t player_no);
  virtual ~connect_player_command();

  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

  virtual void execute();

  static const int identifier = 1;
  virtual uint8_t get_identifier() const {
    return identifier;
  }
};

/**
 * This class is issues when a new entity is to be created (e.g. when a factory has finished building, when a unit is
 * about it fire it's weapon, etc).
 */
class create_entity_command: public command {
private:
  ent::entity_id _entity_id;

public:
  std::string template_name;
  fw::Vector initial_position;
  fw::Vector initial_goal;

  create_entity_command(uint8_t player_no);
  virtual ~create_entity_command();

  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

  virtual void execute();

  static const int identifier = 2;
  virtual uint8_t get_identifier() const {
    return identifier;
  }
};

/**
 * This is the command that is executed when an entity is about to begin executing an order. The actual queuing and
 * so on happens on the client-side and this command is executed only when the command reaches the front of the queue.
 */
class order_command: public command {
public:
  ent::entity_id entity;
  std::shared_ptr<game::order> order;

  order_command(uint8_t player_no);
  virtual ~order_command();

  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

  virtual void execute();

  static const int identifier = 5;
  virtual uint8_t get_identifier() const {
    return identifier;
  }
};

// creates the packet object from the given command identifier
std::shared_ptr<command> create_command(uint8_t id);
std::shared_ptr<command> create_command(uint8_t id, uint8_t player_no);

template<typename T>
std::shared_ptr<T> create_command() {
  return std::dynamic_pointer_cast<T>(create_command(T::identifier));
}

template<typename T>
std::shared_ptr<T> create_command(uint8_t player_no) {
  return std::dynamic_pointer_cast<T>(create_command(T::identifier, player_no));
}

}
