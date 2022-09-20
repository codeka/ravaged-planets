
#include <map>

#include <framework/packet.h>
#include <framework/packet_buffer.h>
#include <framework/logging.h>

namespace fw {
namespace net {

static std::map<uint16_t, std::function<std::shared_ptr<Packet>()>> *packet_registry = nullptr;

Packet::Packet() {
}

Packet::~Packet() {
}

//-------------------------------------------------------------------------
std::shared_ptr<Packet> create_packet(PacketBuffer &buff) {
  std::function<std::shared_ptr<Packet>()> fn = (*packet_registry)[buff.get_packet_type()];
  if (!fn) {
    fw::debug
        << boost::format("  warning: received packet with unknown identifier %1%, dropping.") % buff.get_packet_type()
        << std::endl;
    return std::shared_ptr<Packet>();
  }

  return fn();
}

//-------------------------------------------------------------------------
packet_registrar::packet_registrar(uint16_t id, std::function<std::shared_ptr<Packet>()> fn) {
  if (packet_registry == nullptr)
    packet_registry = new std::map<uint16_t, std::function<std::shared_ptr<Packet>()>>();

  (*packet_registry)[id] = fn;
}

}
}
