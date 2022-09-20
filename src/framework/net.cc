#include <boost/algorithm/string.hpp>

#include <framework/net.h>
#include <framework/logging.h>
#include <framework/exception.h>
#include <framework/packet.h>
#include <framework/packet_buffer.h>

namespace fw {
namespace net {

void initialize() {
  fw::debug << "initializing networking..." << std::endl;
  if (enet_initialize() != 0) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("error initializing ENet"));
  }
}

void destroy() {
  enet_deinitialize();
}

//-------------------------------------------------------------------------
peer::peer(host *hst, ENetPeer *peer, bool connected) :
    _host(hst), _peer(peer), _connected(connected) {
}

peer::~peer() {
  if (_peer != nullptr) {
    // this is technically an error, we'll throw an exception or something (but not yet)
  }
}

void peer::send(packet &pkt, int channel /*= 0*/) {
  packet_buffer buff(pkt.get_identifier());
  pkt.serialize(buff);

  enet_uint32 flags = 0;
  if (pkt.is_essential()) {
    flags |= ENET_PACKET_FLAG_RELIABLE;
  }

  ENetPacket *packet = enet_packet_create(buff.get_buffer(), buff.get_size(), flags);
  enet_peer_send(_peer, static_cast<enet_uint8>(channel), packet);
}

void peer::on_connect() {
  fw::debug << boost::format("connected to peer %1%:%2%") % _peer->address.host % _peer->address.port << std::endl;
  _connected = true;
}

void peer::on_receive(ENetPacket *packet, enet_uint8 /*channel*/) {
  fw::debug << boost::format("packet received from %1%:%2%")
      % _peer->address.host % _peer->address.port << std::endl;

  if (_handler) {
    packet_buffer buff(reinterpret_cast<char *>(packet->data), packet->dataLength);
    std::shared_ptr<net::packet> pkt(create_packet(buff));
    pkt->deserialize(buff);
    _handler(pkt);
  }
}

void peer::on_disconnect() {
  fw::debug << "peer disconnected" << std::endl;
}

//-------------------------------------------------------------------------

host::host() :
    _host(0), _listen_port(0) {
}

host::~host() {
}

bool host::listen(std::string port_range) {
  int min_port, max_port;

  std::vector<std::string> ports;
  boost::split(ports, port_range, boost::algorithm::is_any_of("-"));
  if (ports.size() == 1) {
    min_port = max_port = boost::lexical_cast<int>(ports[0]);
  } else if (ports.size() == 2) {
    min_port = boost::lexical_cast<int>(ports[0]);
    max_port = boost::lexical_cast<int>(ports[1]);
  } else {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Invalid 'port_range': " + port_range));
    min_port = max_port = 0; // stop the "possibly unassigned variable" warning... obviously we never get here, though
  }

  return listen(min_port, max_port);
}

bool host::listen(int min_port, int max_port) {
  for (int port = min_port; port <= max_port; port++) {
    if (try_listen(port)) {
      _listen_port = port;
      return true;
    }
  }

  return false;
}

peer *host::connect(std::string address) {
  if (_host == nullptr)
    try_listen(0);

  std::vector<std::string> parts;
  boost::split(parts, address, boost::algorithm::is_any_of(":"));
  if (parts.size() != 2) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("invalid address, 2 parts expected, IP & port"));
  }

  ENetAddress addr;
  enet_address_set_host(&addr, parts[0].c_str());
  addr.port = boost::lexical_cast<int>(parts[1]);

  // connect to the server (we'll create 10 channels to begin with, maybe that'll change)
  ENetPeer *peer = enet_host_connect(_host, &addr, 10, 0);
  if (peer == nullptr) {
    fw::debug << boost::format("error connecting to peer, no peer object created.") << std::endl;
    return 0;
  }

  net::peer *new_peer = new net::peer(this, peer, false);
  peer->data = new_peer;
  return new_peer;
}

void host::update() {
  if (_host == nullptr)
    return;

  ENetEvent evnt;
  while (enet_host_service(_host, &evnt, 0) > 0) {
    net::peer *peer = static_cast<net::peer *>(evnt.peer->data);

    switch (evnt.type) {
    case ENET_EVENT_TYPE_CONNECT:
      if (peer == nullptr) {
        on_peer_connect(evnt.peer);
      } else {
        peer->on_connect();
      }
      break;

    case ENET_EVENT_TYPE_RECEIVE:
      if (peer != nullptr) {
        peer->on_receive(evnt.packet, evnt.channelID);
      }
      enet_packet_destroy(evnt.packet);
      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      if (peer != nullptr) {
        peer->on_disconnect();
      }
      break;
    }
  }
}

bool host::try_listen(int port) {
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = static_cast<enet_uint16>(port);

  // 32 connections allowed, unlimited bandwidth
  _host = enet_host_create(port == 0 ? 0 : &addr, 32, ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT, 0, 0);
  if (_host == nullptr) {
    fw::debug << boost::format("error listening on port %1%") % port << std::endl;
    return false;
  }

  return true;
}

void host::on_peer_connect(ENetPeer *peer) {
  fw::debug << boost::format("new connection received from %1%:%2%") % peer->address.host % peer->address.port
      << std::endl;

  net::peer *new_peer = new net::peer(this, peer, true);
  _new_connections.push_back(new_peer);
  peer->data = new_peer;
}

std::vector<peer *> host::get_new_connections() {
  std::vector<peer *> new_conns;
  std::swap(new_conns, _new_connections);

  return new_conns;
}

}
}
