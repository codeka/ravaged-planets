#pragma once

#include <memory>

#include <framework/math.h>
#include <framework/status.h>

#include <game/simulation/simulation_thread.h>
#include <game/entities/entity.h>

namespace fw {
namespace net {
class PacketBuffer;
}
}

namespace game {
class Player;
class Order;

// The command is the base class for what the simulation_thread processes. Each turn, we process the commands that
// have been queued up for that turn. Each "action" in the game is represented by a command.
class Command {
private:
  std::shared_ptr<Player> player_;

protected:
  Command(uint8_t player_no);

public:
  virtual ~Command();

  virtual void serialize(fw::net::PacketBuffer &buffer) = 0;
  virtual void deserialize(fw::net::PacketBuffer &buffer) = 0;

  /** Gets an instance of the player who executed this command. */
  std::shared_ptr<Player> const &get_player() const {
    return player_;
  }

  /** When it's our turn, this'll get called to actually execute the command. */
  virtual void execute() = 0;

  /**
   * Gets the identifier of this command, which is used to differentiate between all the commands
   * in the game (e.g. move vs. attack vs. build, etc)
   */
  virtual uint8_t get_identifier() const = 0;
};

// This command is sent to another player when we want to notify them about a new player (i.e. when we first connect
//to them [as a non-Host], or when we add a new AI player)
class ConnectPlayerCommand: public Command {
private:

public:
  ConnectPlayerCommand(uint8_t player_no);
  virtual ~ConnectPlayerCommand();

  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

  virtual void execute();

  static const int identifier = 1;
  virtual uint8_t get_identifier() const {
    return identifier;
  }
};

// This class is issues when a new Entity is to be created (e.g. when a factory has finished building, when a unit is
// about it fire it's weapon, etc).
class CreateEntityCommand: public Command {
private:
  ent::entity_id entity_id_;

public:
  std::string template_name;
  fw::Vector initial_position;
  fw::Vector initial_goal;

  CreateEntityCommand(uint8_t player_no);
  virtual ~CreateEntityCommand();

  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

  virtual void execute();

  static const int identifier = 2;
  virtual uint8_t get_identifier() const {
    return identifier;
  }
};

// This is the command that is executed when an Entity is about to begin executing an order. The actual queuing and
// so on happens on the client-side and this command is executed only when the command reaches the front of the queue.
class OrderCommand: public Command {
public:
  ent::entity_id Entity;
  std::shared_ptr<game::Order> order;

  OrderCommand(uint8_t player_no);
  virtual ~OrderCommand();

  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

  virtual void execute();

  static const int identifier = 5;
  virtual uint8_t get_identifier() const {
    return identifier;
  }
};

// creates the Packet object from the given command identifier
fw::StatusOr<std::shared_ptr<Command>> CreateCommand(uint8_t id);
fw::StatusOr<std::shared_ptr<Command>> CreateCommand(uint8_t id, uint8_t player_no);

template<typename T>
std::shared_ptr<T> create_command() {
  auto cmd = CreateCommand(T::identifier);
  if (!cmd.ok()) {
    LOG(ERR) << cmd.status() << std::endl;
    std::terminate();
  }
  return std::dynamic_pointer_cast<T>(*cmd);
}

template<typename T>
std::shared_ptr<T> create_command(uint8_t player_no) {
  auto cmd = CreateCommand(T::identifier, player_no);
  if (!cmd.ok()) {
    LOG(ERR) << cmd.status() << std::endl;
    std::terminate();
  }
  return std::dynamic_pointer_cast<T>(*cmd);
}

}
