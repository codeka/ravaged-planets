//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include <framework/packet.h>
#include <framework/color.h>

namespace fw {
namespace net {
class PacketBuffer;
}
}

namespace game {
class Command;

/**
 * This is the first Packet you send to a Host when you want to join the game. we'll response with a join_response
 * detailing the players that already exist in the game, the initial state and so on.
 */
class JoinRequestPacket: public fw::net::Packet {
private:
  uint32_t user_id_;
  fw::Color color_;

protected:
  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

public:
  JoinRequestPacket();
  virtual ~JoinRequestPacket();

  void set_user_id(uint32_t value) {
    user_id_ = value;
  }
  uint32_t get_user_id() const {
    return user_id_;
  }

  // gets or sets the color of the connecting player (only useful when connecting
  // to a Peer, rather than the Host)
  void set_color(fw::Color value) {
    color_ = value;
  }
  fw::Color get_color() const {
    return color_;
  }

  static const int identifier = 1;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// this is the response from a join request
class JoinResponsePacket: public fw::net::Packet {
private:
  std::string map_name_;
  std::vector<uint32_t> other_users_;
  fw::Color my_color_;
  fw::Color your_color_;

protected:
  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

public:
  JoinResponsePacket();
  virtual ~JoinResponsePacket();

  void set_map_name(std::string const &value) {
    map_name_ = value;
  }
  std::string const &get_map_name() const {
    return map_name_;
  }

  std::vector<uint32_t> &get_other_users() {
    return other_users_;
  }

  // gets or sets the color that the player you just connected to has
  fw::Color get_my_color() const {
    return my_color_;
  }
  void set_my_color(fw::Color col) {
    my_color_ = col;
  }

  // gets or sets the color we'll allow you to have (this is only useful when coming
  // from the Host of the game - you can ignore it from other peers)
  fw::Color get_your_color() const {
    return your_color_;
  }
  void set_your_color(fw::Color col) {
    your_color_ = col;
  }

  static const int identifier = 2;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// a chat Packet contains a short textual message to display for this player
class ChatPacket: public fw::net::Packet {
private:
  std::string msg_;

protected:
  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

public:
  ChatPacket();
  virtual ~ChatPacket();

  void set_msg(std::string const &value) {
    msg_ = value;
  }
  std::string const &get_msg() const {
    return msg_;
  }

  static const int identifier = 3;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// this Packet is sent to use when our Peer is ready to start the game
class StartGamePacket: public fw::net::Packet {
protected:
  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

public:
  StartGamePacket();
  virtual ~StartGamePacket();

  static const int identifier = 4;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// this Packet is sent at the end of each turn and notifies our Peer
// of our commands that are queued for the next turn
class CommandPacket: public fw::net::Packet {
private:
  std::vector<std::shared_ptr<Command>> commands_;

protected:
  virtual void serialize(fw::net::PacketBuffer &buffer);
  virtual void deserialize(fw::net::PacketBuffer &buffer);

public:
  CommandPacket();
  virtual ~CommandPacket();

  void set_commands(std::vector<std::shared_ptr<Command>> &commands) {
    commands_ = commands;
  }
  std::vector<std::shared_ptr<Command>> &get_commands() {
    return commands_;
  }

  static const int identifier = 5;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

}
