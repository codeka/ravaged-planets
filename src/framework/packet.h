#pragma once

#include <functional>
#include <memory>

#include <framework/packet_buffer.h>

namespace fw::net {
class net_peer;

// This is a helper macro for registering packet types with the packet_factory
#define PACKET_REGISTER(type) \
  std::shared_ptr<fw::net::packet> create_ ## type () { return std::shared_ptr<fw::net::packet>(new type()); } \
  fw::net::packet_registrar reg_ ## type(type::identifier, std::bind(&create_ ## type))

/** The base packet class which represents the packets we send to/from our remote peers. */
class packet {
protected:
  friend class peer;

  // serialize/deserialize ourselves to/from the given packet_buffer
  virtual void serialize(packet_buffer &buffer) = 0;
  virtual void deserialize(packet_buffer &buffer) = 0;

  packet();
public:
  virtual ~packet();
  virtual uint16_t get_identifier() const = 0;

  /** Gets a value which indicates whether this packet is "essential" (default is true). */
  virtual bool is_essential() {
    return true;
  }
};

/** Creates the packet object from the given packet_buffer. */
std::shared_ptr<packet> create_packet(packet_buffer &buff);

/** Helper class that you use indirectly via the PACKET_REGISTER macro to register a packet with the packet_factory. */
class packet_registrar {
public:
  packet_registrar(uint16_t id, std::function<std::shared_ptr<packet>()> fn);
};

}
