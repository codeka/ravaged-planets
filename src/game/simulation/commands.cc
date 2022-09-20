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
  std::shared_ptr<command> create_ ## type (uint8_t player_no) { \
    return std::shared_ptr<command>(new type(player_no)); \
  } \
  command_registrar reg_ ## type(type::identifier, &create_ ## type)

typedef std::function<std::shared_ptr<command>(uint8_t)> create_command_fn;
typedef std::map<uint8_t, create_command_fn> command_registry_map;
static command_registry_map *g_command_registry = nullptr;

// this is a helper class that you use indirectly via the COMMAND_REGISTER macro
// to register a command.
class command_registrar {
public:
  command_registrar(uint8_t id, create_command_fn fn);
};

command_registrar::command_registrar(uint8_t id, create_command_fn fn) {
  if (g_command_registry == nullptr)
    g_command_registry = new command_registry_map();

  (*g_command_registry)[id] = fn;
}

// creates the packet object from the given command identifier
std::shared_ptr<command> create_command(uint8_t id, uint8_t player_no) {
  create_command_fn fn = (*g_command_registry)[id];
  if (fn == nullptr) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("command does not exist: " + boost::lexical_cast<std::string>(id)));
  }

  return fn(player_no);
}

// creates the packet object from the given command identifier
std::shared_ptr<command> create_command(uint8_t id) {
  create_command_fn fn = (*g_command_registry)[id];
  if (fn == nullptr) {
    BOOST_THROW_EXCEPTION(
        fw::Exception() << fw::message_error_info("command does not exist: " + boost::lexical_cast<std::string>(id)));
  }

  return fn(simulation_thread::get_instance()->get_local_player()->get_player_no());
}

//-------------------------------------------------------------------------

COMMAND_REGISTER(connect_player_command);
COMMAND_REGISTER(create_entity_command);
COMMAND_REGISTER(order_command);

//-------------------------------------------------------------------------

command::command(uint8_t player_no) {
  _player = simulation_thread::get_instance()->get_player(player_no);
}

command::~command() {
}

//-------------------------------------------------------------------------

connect_player_command::connect_player_command(uint8_t player_no) :
    command(player_no) {
}

connect_player_command::~connect_player_command() {
}

void connect_player_command::serialize(fw::net::packet_buffer &) {
}

void connect_player_command::deserialize(fw::net::packet_buffer &) {
}

void connect_player_command::execute() {
}

//-------------------------------------------------------------------------
static volatile std::atomic<ent::entity_id> g_next_entity_number;

ent::entity_id generate_entity_id() {
  uint32_t entity_id = ++g_next_entity_number;
  uint8_t player_id = simulation_thread::get_instance()->get_local_player()->get_player_no();

  if (entity_id > 0x00ffffff) {
    // this is probably pretty bad! I can't imagine a game that needs 16 million entities...
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Too many entities have been created!!"));
  }

  return static_cast<ent::entity_id>((static_cast<uint32_t>(player_id) << 24) | entity_id);
}

create_entity_command::create_entity_command(uint8_t player_no) :
    command(player_no) {
  _entity_id = generate_entity_id();
}

create_entity_command::~create_entity_command() {
}

void create_entity_command::serialize(fw::net::packet_buffer &buffer) {
  buffer << _entity_id;
  buffer << template_name;
  buffer << initial_position;
  buffer << initial_goal;
}

void create_entity_command::deserialize(fw::net::packet_buffer &buffer) {
  buffer >> _entity_id;
  buffer >> template_name;
  buffer >> initial_position;
  buffer >> initial_goal;
}

void create_entity_command::execute() {
  ent::entity_manager *ent_mgr = game::world::get_instance()->get_entity_manager();
  std::shared_ptr<ent::entity> ent = ent_mgr->create_entity(template_name, static_cast<ent::entity_id>(_entity_id));

  ent::position_component *position = ent->get_component<ent::position_component>();
  if (position != nullptr) {
    position->set_position(initial_position);

    ent::moveable_component *moveable = ent->get_component<ent::moveable_component>();
    if (moveable != nullptr) {
      fw::Vector goal = position->get_position() + (initial_goal - initial_position);
      moveable->set_goal(goal, true /* skip_pathing */);
    }
  }

  ent::ownable_component *ownable = ent->get_component<ent::ownable_component>();
  if (ownable != nullptr) {
    ownable->set_owner(get_player());
  }
}

//-------------------------------------------------------------------------
order_command::order_command(uint8_t player_no) :
    command(player_no), entity(0) {
}

order_command::~order_command() {
}

void order_command::serialize(fw::net::packet_buffer &buffer) {
  buffer << entity;
  buffer << order->get_identifier();
  order->serialize(buffer);
}

void order_command::deserialize(fw::net::packet_buffer &buffer) {
  buffer >> entity;

  uint16_t order_id;
  buffer >> order_id;

  order = create_order(order_id);
  order->deserialize(buffer);
}

void order_command::execute() {
  ent::entity_manager *ent_mgr = game::world::get_instance()->get_entity_manager();
  std::shared_ptr<ent::entity> ent = ent_mgr->get_entity(entity).lock();
  if (ent) {
    ent::orderable_component *orderable = ent->get_component<ent::orderable_component>();
    if (orderable != 0) {
      orderable->execute_order(order);
    }
  }
}

}
