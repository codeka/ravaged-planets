//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//
#pragma once

#include <framework/packet.h>
#include <framework/color.h>

namespace fw {
namespace net {
class packet_buffer;
}
}

namespace game {
class command;

/**
 * This is the first packet you send to a host when you want to join the game. we'll response with a join_response
 * detailing the players that already exist in the game, the initial state and so on.
 */
class join_request_packet: public fw::net::packet {
private:
  uint32_t _user_id;
  fw::Color color_;

protected:
  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

public:
  join_request_packet();
  virtual ~join_request_packet();

  void set_user_id(uint32_t value) {
    _user_id = value;
  }
  uint32_t get_user_id() const {
    return _user_id;
  }

  // gets or sets the color of the connecting player (only useful when connecting
  // to a peer, rather than the host)
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
class join_response_packet: public fw::net::packet {
private:
  std::string _map_name;
  std::vector<uint32_t> _other_users;
  fw::Color _my_color;
  fw::Color _your_color;

protected:
  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

public:
  join_response_packet();
  virtual ~join_response_packet();

  void set_map_name(std::string const &value) {
    _map_name = value;
  }
  std::string const &get_map_name() const {
    return _map_name;
  }

  std::vector<uint32_t> &get_other_users() {
    return _other_users;
  }

  // gets or sets the color that the player you just connected to has
  fw::Color get_my_color() const {
    return _my_color;
  }
  void set_my_color(fw::Color col) {
    _my_color = col;
  }

  // gets or sets the color we'll allow you to have (this is only useful when coming
  // from the host of the game - you can ignore it from other peers)
  fw::Color get_your_color() const {
    return _your_color;
  }
  void set_your_color(fw::Color col) {
    _your_color = col;
  }

  static const int identifier = 2;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// a chat packet contains a short textual message to display for this player
class chat_packet: public fw::net::packet {
private:
  std::string _msg;

protected:
  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

public:
  chat_packet();
  virtual ~chat_packet();

  void set_msg(std::string const &value) {
    _msg = value;
  }
  std::string const &get_msg() const {
    return _msg;
  }

  static const int identifier = 3;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// this packet is sent to use when our peer is ready to start the game
class start_game_packet: public fw::net::packet {
protected:
  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

public:
  start_game_packet();
  virtual ~start_game_packet();

  static const int identifier = 4;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

// this packet is sent at the end of each turn and notifies our peer
// of our commands that are queued for the next turn
class command_packet: public fw::net::packet {
private:
  std::vector<std::shared_ptr<command> > _commands;

protected:
  virtual void serialize(fw::net::packet_buffer &buffer);
  virtual void deserialize(fw::net::packet_buffer &buffer);

public:
  command_packet();
  virtual ~command_packet();

  void set_commands(std::vector<std::shared_ptr<command>> &commands) {
    _commands = commands;
  }
  std::vector<std::shared_ptr<command>> &get_commands() {
    return _commands;
  }

  static const int identifier = 5;
  virtual uint16_t get_identifier() const {
    return identifier;
  }
};

}
