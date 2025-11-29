#include <atomic>
#include <functional>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/packet_buffer.h>

#include <game/simulation/commands.h>
#include <game/simulation/player.h>
#include <game/simulation/local_player.h>
#include <game/simulation/orders.h>
#include <game/simulation/simulation_thread.h>
#include <game/entities/entity.h>
#include <game/entities/entity_manager.h>
#include <game/entities/position_component.h>
#include <game/entities/ownable_component.h>
#include <game/entities/orderable_component.h>
#include <game/entities/moveable_component.h>
#include <game/entities/builder_component.h>
#include <game/entities/weapon_component.h>
#include <game/world/world.h>

namespace game {

// this is a helper macro for registering command types with the command_factory
#define COMMAND_REGISTER(type) \
  std::shared_ptr<Command> create_ ## type (uint8_t player_no) { \
    return std::shared_ptr<Command>(new type(player_no)); \
  } \
  CommandRegistrar reg_ ## type(type::identifier, &create_ ## type)

typedef std::function<std::shared_ptr<Command>(uint8_t)> create_command_fn;
typedef std::map<uint8_t, create_command_fn> command_registry_map;
static command_registry_map *g_command_registry = nullptr;

// this is a helper class that you use indirectly via the COMMAND_REGISTER macro
// to register a command.
class CommandRegistrar {
public:
  CommandRegistrar(uint8_t id, create_command_fn fn);
};

CommandRegistrar::CommandRegistrar(uint8_t id, create_command_fn fn) {
  if (g_command_registry == nullptr)
    g_command_registry = new command_registry_map();

  (*g_command_registry)[id] = fn;
}

// creates the Packet object from the given command identifier
std::shared_ptr<Command> create_command(uint8_t id, uint8_t player_no) {
  create_command_fn fn = (*g_command_registry)[id];
  if (fn == nullptr) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("command does not exist: " + boost::lexical_cast<std::string>(id)));
  }

  return fn(player_no);
}

// creates the Packet object from the given command identifier
std::shared_ptr<Command> create_command(uint8_t id) {
  create_command_fn fn = (*g_command_registry)[id];
  if (fn == nullptr) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("command does not exist: " + boost::lexical_cast<std::string>(id)));
  }

  return fn(SimulationThread::get_instance()->get_local_player()->get_player_no());
}

//-------------------------------------------------------------------------

COMMAND_REGISTER(ConnectPlayerCommand);
COMMAND_REGISTER(CreateEntityCommand);
COMMAND_REGISTER(OrderCommand);

//-------------------------------------------------------------------------

Command::Command(uint8_t player_no) {
  player_ = SimulationThread::get_instance()->get_player(player_no);
}

Command::~Command() {
}

//-------------------------------------------------------------------------

ConnectPlayerCommand::ConnectPlayerCommand(uint8_t player_no) :
    Command(player_no) {
}

ConnectPlayerCommand::~ConnectPlayerCommand() {
}

void ConnectPlayerCommand::serialize(fw::net::PacketBuffer &) {
}

void ConnectPlayerCommand::deserialize(fw::net::PacketBuffer &) {
}

void ConnectPlayerCommand::execute() {
}

//-------------------------------------------------------------------------
static volatile std::atomic<ent::entity_id> g_next_entity_number;

ent::entity_id generate_entity_id() {
  uint32_t entity_id = ++g_next_entity_number;
  uint8_t player_id = SimulationThread::get_instance()->get_local_player()->get_player_no();

  if (entity_id > 0x00ffffff) {
    // this is probably pretty bad! I can't imagine a game that needs 16 million entities.
    fw::debug << "ERROR - too many entities have been created!!" << std::endl;
    std::terminate(); // TODO: should be an exception after all?
  }

  // TODO: this could be spoofed by a malicous peer.
  return static_cast<ent::entity_id>((static_cast<uint32_t>(player_id) << 24) | entity_id);
}

CreateEntityCommand::CreateEntityCommand(uint8_t player_no) :
    Command(player_no) {
  entity_id_ = generate_entity_id();
}

CreateEntityCommand::~CreateEntityCommand() {
}

void CreateEntityCommand::serialize(fw::net::PacketBuffer &buffer) {
  buffer << entity_id_;
  buffer << template_name;
  buffer << initial_position;
  buffer << initial_goal;
}

void CreateEntityCommand::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> entity_id_;
  buffer >> template_name;
  buffer >> initial_position;
  buffer >> initial_goal;
}

void CreateEntityCommand::execute() {
  ent::EntityManager *ent_mgr = game::World::get_instance()->get_entity_manager();
  std::shared_ptr<ent::Entity> ent =
      ent_mgr->create_entity(template_name, static_cast<ent::entity_id>(entity_id_));

  ent::PositionComponent *position = ent->get_component<ent::PositionComponent>();
  if (position != nullptr) {
    position->set_position(initial_position);

    ent::MoveableComponent *moveable = ent->get_component<ent::MoveableComponent>();
    if (moveable != nullptr) {
      fw::Vector goal = position->get_position() + (initial_goal - initial_position);
      moveable->set_goal(goal, true /* skip_pathing */);
    }
  }

  ent::OwnableComponent *ownable = ent->get_component<ent::OwnableComponent>();
  if (ownable != nullptr) {
    ownable->set_owner(get_player());
  }
}

//-------------------------------------------------------------------------
OrderCommand::OrderCommand(uint8_t player_no) :
    Command(player_no), Entity(0) {
}

OrderCommand::~OrderCommand() {
}

void OrderCommand::serialize(fw::net::PacketBuffer &buffer) {
  buffer << Entity;
  buffer << order->get_identifier();
  order->serialize(buffer);
}

void OrderCommand::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> Entity;

  uint16_t order_id;
  buffer >> order_id;

  order = create_order(order_id);
  order->deserialize(buffer);
}

void OrderCommand::execute() {
  ent::EntityManager *ent_mgr = game::World::get_instance()->get_entity_manager();
  std::shared_ptr<ent::Entity> ent = ent_mgr->get_entity(Entity).lock();
  if (ent) {
    ent::OrderableComponent *orderable = ent->get_component<ent::OrderableComponent>();
    if (orderable != 0) {
      orderable->execute_order(order);
    }
  }
}

}
