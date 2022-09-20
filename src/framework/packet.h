#pragma once

#include <functional>
#include <memory>

#include <framework/packet_buffer.h>

namespace fw::net {
class net_peer;

// This is a helper macro for registering Packet types with the packet_factory
#define PACKET_REGISTER(type) \
  std::shared_ptr<fw::net::Packet> create_ ## type () { return std::shared_ptr<fw::net::Packet>(new type()); } \
  fw::net::packet_registrar reg_ ## type(type::identifier, std::bind(&create_ ## type))

// The base Packet class which represents the packets we send to/from our remote peers.
class Packet {
protected:
  friend class Peer;

  // serialize/deserialize ourselves to/from the given PacketBuffer
  virtual void serialize(PacketBuffer &buffer) = 0;
  virtual void deserialize(PacketBuffer &buffer) = 0;

  Packet();
public:
  virtual ~Packet();
  virtual uint16_t get_identifier() const = 0;

  // Gets a value which indicates whether this Packet is "essential" (meaning cannot be dropped, default is true).
  virtual bool is_essential() {
    return true;
  }
};

// Creates the Packet object from the given PacketBuffer.
std::shared_ptr<Packet> create_packet(PacketBuffer &buff);

// Helper class that you use indirectly via the PACKET_REGISTER macro to register a Packet with the packet_factory.
class packet_registrar {
public:
  packet_registrar(uint16_t id, std::function<std::shared_ptr<Packet>()> fn);
};

}
