
#include <framework/packet_buffer.h>
#include <framework/exception.h>

#include <game/simulation/packets.h>
#include <game/simulation/commands.h>
#include <game/simulation/player.h>

namespace game {

PACKET_REGISTER(JoinRequestPacket);
PACKET_REGISTER(JoinResponsePacket);
PACKET_REGISTER(ChatPacket);
PACKET_REGISTER(StartGamePacket);
PACKET_REGISTER(CommandPacket);

//-------------------------------------------------------------------------

JoinRequestPacket::JoinRequestPacket() : user_id_(-1) {
}

JoinRequestPacket::~JoinRequestPacket() {
}

void JoinRequestPacket::serialize(fw::net::PacketBuffer &buffer) {
  buffer << user_id_;
  buffer << color_;
}

void JoinRequestPacket::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> user_id_;
  buffer >> color_;
}

//-------------------------------------------------------------------------

JoinResponsePacket::JoinResponsePacket() {
}

JoinResponsePacket::~JoinResponsePacket() {
}

void JoinResponsePacket::serialize(fw::net::PacketBuffer &buffer) {
  buffer << map_name_;
  buffer << other_users_.size();
  for (uint32_t sess_id : other_users_) {
    buffer << sess_id;
  }
  buffer << my_color_;
  buffer << your_color_;
}

void JoinResponsePacket::deserialize(fw::net::PacketBuffer &buffer) {
  typedef std::vector<std::string>::size_type size_type;

  buffer >> map_name_;
  size_type num_other_users;
  buffer >> num_other_users;
  for (size_type i = 0; i < num_other_users; i++) {
    uint32_t other_user;
    buffer >> other_user;

    other_users_.push_back(other_user);
  }
  buffer >> my_color_;
  buffer >> your_color_;
}

//-------------------------------------------------------------------------
ChatPacket::ChatPacket() :
    msg_("") {
}

ChatPacket::~ChatPacket() {
}

void ChatPacket::serialize(fw::net::PacketBuffer &buffer) {
  buffer << msg_;
}

void ChatPacket::deserialize(fw::net::PacketBuffer &buffer) {
  buffer >> msg_;
}

//-------------------------------------------------------------------------

StartGamePacket::StartGamePacket() {
}

StartGamePacket::~StartGamePacket() {
}

void StartGamePacket::serialize(fw::net::PacketBuffer &) {
}

void StartGamePacket::deserialize(fw::net::PacketBuffer &) {
}

//----------------------------------------------------------------------------

CommandPacket::CommandPacket() {
}

CommandPacket::~CommandPacket() {
}

void CommandPacket::serialize(fw::net::PacketBuffer &buffer) {
  uint8_t num_commands = static_cast<uint8_t>(commands_.size());
  buffer << num_commands;

  for (std::shared_ptr<Command> &cmd : commands_) {
    buffer << cmd->get_identifier();

    if (cmd->get_player() != nullptr) {
      buffer << cmd->get_player()->get_player_no();
    } else {
      buffer << 0;
    }

    cmd->serialize(buffer);
  }
}

void CommandPacket::deserialize(fw::net::PacketBuffer &buffer) {
  uint8_t num_commands;
  buffer >> num_commands;

  commands_.clear();
  for (uint8_t i = 0; i < num_commands; i++) {
    uint8_t id;
    uint8_t player_no;
    buffer >> id;
    buffer >> player_no;

    auto cmd = CreateCommand(id, player_no);
    if (!cmd.ok()) {
      // TODO: return error
      fw::debug << "ERROR deserializing command packet: " << cmd.status() << std::endl;
    } else {
      (*cmd)->deserialize(buffer);
      commands_.push_back(*cmd);
    }
  }
}

}
