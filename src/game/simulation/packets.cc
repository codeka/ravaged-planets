#include <boost/foreach.hpp>

#include <framework/packet_buffer.h>
#include <framework/exception.h>

#include <game/simulation/packets.h>
#include <game/simulation/commands.h>
#include <game/simulation/player.h>

namespace game {

PACKET_REGISTER(join_request_packet);
PACKET_REGISTER(join_response_packet);
PACKET_REGISTER(chat_packet);
PACKET_REGISTER(start_game_packet);
PACKET_REGISTER(command_packet);

//-------------------------------------------------------------------------

join_request_packet::join_request_packet() : _user_id(-1) {
}

join_request_packet::~join_request_packet() {
}

void join_request_packet::serialize(fw::net::packet_buffer &buffer) {
  buffer << _user_id;
  buffer << _color;
}

void join_request_packet::deserialize(fw::net::packet_buffer &buffer) {
  buffer >> _user_id;
  buffer >> _color;
}

//-------------------------------------------------------------------------

join_response_packet::join_response_packet() {
}

join_response_packet::~join_response_packet() {
}

void join_response_packet::serialize(fw::net::packet_buffer &buffer) {
  buffer << _map_name;
  buffer << _other_users.size();
  BOOST_FOREACH(uint32_t sess_id, _other_users) {
    buffer << sess_id;
  }
  buffer << _my_color;
  buffer << _your_color;
}

void join_response_packet::deserialize(fw::net::packet_buffer &buffer) {
  typedef std::vector<std::string>::size_type size_type;

  buffer >> _map_name;
  size_type num_other_users;
  buffer >> num_other_users;
  for (size_type i = 0; i < num_other_users; i++) {
    uint32_t other_user;
    buffer >> other_user;

    _other_users.push_back(other_user);
  }
  buffer >> _my_color;
  buffer >> _your_color;
}

//-------------------------------------------------------------------------
chat_packet::chat_packet() :
    _msg("") {
}

chat_packet::~chat_packet() {
}

void chat_packet::serialize(fw::net::packet_buffer &buffer) {
  buffer << _msg;
}

void chat_packet::deserialize(fw::net::packet_buffer &buffer) {
  buffer >> _msg;
}

//-------------------------------------------------------------------------

start_game_packet::start_game_packet() {
}

start_game_packet::~start_game_packet() {
}

void start_game_packet::serialize(fw::net::packet_buffer &) {
}

void start_game_packet::deserialize(fw::net::packet_buffer &) {
}

//----------------------------------------------------------------------------

command_packet::command_packet() {
}

command_packet::~command_packet() {
}

void command_packet::serialize(fw::net::packet_buffer &buffer) {
  uint8_t num_commands = static_cast<uint8_t>(_commands.size());
  buffer << num_commands;

  BOOST_FOREACH(std::shared_ptr<command> &cmd, _commands) {
    buffer << cmd->get_identifier();

    if (cmd->get_player() != nullptr) {
      buffer << cmd->get_player()->get_player_no();
    } else {
      buffer << 0;
    }

    cmd->serialize(buffer);
  }
}

void command_packet::deserialize(fw::net::packet_buffer &buffer) {
  uint8_t num_commands;
  buffer >> num_commands;

  _commands.clear();
  for (uint8_t i = 0; i < num_commands; i++) {
    uint8_t id;
    uint8_t player_no;
    buffer >> id;
    buffer >> player_no;

    std::shared_ptr<command> cmd = create_command(id, player_no);
    cmd->deserialize(buffer);

    _commands.push_back(cmd);
  }
}

}
