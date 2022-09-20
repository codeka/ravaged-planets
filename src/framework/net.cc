#include <boost/algorithm/string.hpp>

#include <framework/net.h>
#include <framework/logging.h>
#include <framework/exception.h>
#include <framework/packet.h>
#include <framework/packet_buffer.h>

namespace fw::net {

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
Peer::Peer(Host *hst, ENetPeer *Peer, bool connected) :
    host_(hst), peer_(Peer), connected_(connected) {
}

Peer::~Peer() {
  if (peer_ != nullptr) {
    // this is technically an error, we'll throw an exception or something (but not yet)
  }
}

void Peer::send(Packet &pkt, int channel /*= 0*/) {
  PacketBuffer buff(pkt.get_identifier());
  pkt.serialize(buff);

  enet_uint32 flags = 0;
  if (pkt.is_essential()) {
    flags |= ENET_PACKET_FLAG_RELIABLE;
  }

  ENetPacket *Packet = enet_packet_create(buff.get_buffer(), buff.get_size(), flags);
  enet_peer_send(peer_, static_cast<enet_uint8>(channel), Packet);
}

void Peer::on_connect() {
  fw::debug << boost::format("connected to peer %1%:%2%") % peer_->address.host % peer_->address.port << std::endl;
  connected_ = true;
}

void Peer::on_receive(ENetPacket *Packet, enet_uint8 /*channel*/) {
  fw::debug << boost::format("packet received from %1%:%2%")
      % peer_->address.host % peer_->address.port << std::endl;

  if (handler_) {
    PacketBuffer buff(reinterpret_cast<char *>(Packet->data), Packet->dataLength);
    std::shared_ptr<net::Packet> pkt(create_packet(buff));
    pkt->deserialize(buff);
    handler_(pkt);
  }
}

void Peer::on_disconnect() {
  fw::debug << "peer disconnected" << std::endl;
}

//-------------------------------------------------------------------------

Host::Host() :
    host_(0), _listen_port(0) {
}

Host::~Host() {
}

bool Host::listen(std::string port_range) {
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

bool Host::listen(int min_port, int max_port) {
  for (int port = min_port; port <= max_port; port++) {
    if (try_listen(port)) {
      _listen_port = port;
      return true;
    }
  }

  return false;
}

Peer *Host::connect(std::string address) {
  if (host_ == nullptr)
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
  ENetPeer *Peer = enet_host_connect(host_, &addr, 10, 0);
  if (Peer == nullptr) {
    fw::debug << boost::format("error connecting to peer, no peer object created.") << std::endl;
    return 0;
  }

  net::Peer *new_peer = new net::Peer(this, Peer, false);
  Peer->data = new_peer;
  return new_peer;
}

void Host::update() {
  if (host_ == nullptr)
    return;

  ENetEvent evnt;
  while (enet_host_service(host_, &evnt, 0) > 0) {
    net::Peer *Peer = static_cast<net::Peer *>(evnt.peer->data);

    switch (evnt.type) {
    case ENET_EVENT_TYPE_CONNECT:
      if (Peer == nullptr) {
        on_peer_connect(evnt.peer);
      } else {
        Peer->on_connect();
      }
      break;

    case ENET_EVENT_TYPE_RECEIVE:
      if (Peer != nullptr) {
        Peer->on_receive(evnt.packet, evnt.channelID);
      }
      enet_packet_destroy(evnt.packet);
      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      if (Peer != nullptr) {
        Peer->on_disconnect();
      }
      break;
    }
  }
}

bool Host::try_listen(int port) {
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = static_cast<enet_uint16>(port);

  // 32 connections allowed, unlimited bandwidth
  host_ = enet_host_create(port == 0 ? 0 : &addr, 32, ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT, 0, 0);
  if (host_ == nullptr) {
    fw::debug << boost::format("error listening on port %1%") % port << std::endl;
    return false;
  }

  return true;
}

void Host::on_peer_connect(ENetPeer *Peer) {
  fw::debug << boost::format("new connection received from %1%:%2%") % Peer->address.host % Peer->address.port
      << std::endl;

  net::Peer *new_peer = new net::Peer(this, Peer, true);
  _new_connections.push_back(new_peer);
  Peer->data = new_peer;
}

std::vector<Peer *> Host::get_new_connections() {
  std::vector<Peer *> new_conns;
  std::swap(new_conns, _new_connections);

  return new_conns;
}

}
