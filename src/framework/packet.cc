
#include <framework/packet.h>
#include <framework/packet_buffer.h>
#include <framework/logging.h>

namespace fw {
namespace net {

static std::map<uint16_t, std::function<std::shared_ptr<packet>()>> *packet_registry = nullptr;

packet::packet() {
}

packet::~packet() {
}

//-------------------------------------------------------------------------
std::shared_ptr<packet> create_packet(packet_buffer &buff) {
  std::function<std::shared_ptr<packet>()> fn = (*packet_registry)[buff.get_packet_type()];
  if (!fn) {
    fw::debug
        << boost::format("  warning: received packet with unknown identifier %1%, dropping.") % buff.get_packet_type()
        << std::endl;
    return std::shared_ptr<packet>();
  }

  return fn();
}

//-------------------------------------------------------------------------
packet_registrar::packet_registrar(uint16_t id, std::function<std::shared_ptr<packet>()> fn) {
  if (packet_registry == nullptr)
    packet_registry = new std::map<uint16_t, std::function<std::shared_ptr<packet>()>>();

  (*packet_registry)[id] = fn;
}

}
}
