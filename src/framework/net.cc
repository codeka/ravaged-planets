#include <framework/net.h>

#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_split.h>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/packet.h>
#include <framework/packet_buffer.h>
#include <framework/status.h>

namespace fw::net {

fw::Status initialize() {
  fw::debug << "initializing networking..." << std::endl;
  if (enet_initialize() != 0) {
    return fw::ErrorStatus("error initializing ENet");
  }

  return fw::OkStatus();
}

void destroy() {
  enet_deinitialize();
}

//-------------------------------------------------------------------------
Peer::Peer(Host *host, ENetPeer *peer, bool connected) :
    host_(host), peer_(peer), connected_(connected) {
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
  fw::debug << "connected to peer " << peer_->address.host << ":" << peer_->address.port
            << std::endl;
  connected_ = true;
}

void Peer::on_receive(ENetPacket *Packet, enet_uint8 /*channel*/) {
  fw::debug << "packet received from " << peer_->address.host << ":" << peer_->address.port
            << std::endl;

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
    host_(0), listen_port_(0) {
}

Host::~Host() {
}

fw::Status Host::listen(std::string port_range) {
  int min_port, max_port;

  std::vector<std::string> ports = absl::StrSplit(port_range, "-");
  if (ports.size() == 1) {
    if (!absl::SimpleAtoi(ports[0], &min_port)) {
      return fw::ErrorStatus("invalid port: ") << ports[0];
    }
    max_port = min_port;
  } else if (ports.size() == 2) {
    if (!absl::SimpleAtoi(ports[0], &min_port)) {
      return fw::ErrorStatus("invalid port: ") << ports[0];
    }
    if (!absl::SimpleAtoi(ports[1], &max_port)) {
      return fw::ErrorStatus("invalid port: ") << ports[1];
    }
  } else {
    return fw::ErrorStatus("Invalid 'port_range': ") << port_range;
  }

  return listen(min_port, max_port);
}

fw::Status Host::listen(int min_port, int max_port) {
  for (int port = min_port; port <= max_port; port++) {
    if (try_listen(port).ok()) {
      listen_port_ = port;
      return fw::OkStatus();
    }
  }

  return fw::ErrorStatus(
    absl::StrCat("unable to listen on any port min=", min_port, " max=", max_port));
}

fw::StatusOr<std::shared_ptr<Peer>> Host::connect(std::string address) {
  if (host_ == nullptr) {
    RETURN_IF_ERROR(try_listen(0));
  }

  std::vector<std::string> parts = absl::StrSplit(address, ":");
  if (parts.size() != 2) {
    return fw::ErrorStatus("invalid address, 2 parts expected, IP & port: ") << address;
  }

  ENetAddress addr;
  enet_address_set_host(&addr, parts[0].c_str());
  if (!absl::SimpleAtoi(parts[1], &addr.port)) {
    return fw::ErrorStatus("invalid port: ") << parts[1];
  }

  // connect to the server (we'll create 10 channels to begin with, maybe that'll change)
  ENetPeer *enet_peer = enet_host_connect(host_, &addr, 10, 0);
  if (enet_peer == nullptr) {
    return fw::ErrorStatus("error connecting to peer");
  }

  auto peer = std::make_shared<Peer>(this, enet_peer, false);
  enet_peer->data = peer.get();
  return peer;
}

void Host::update() {
  if (host_ == nullptr)
    return;

  ENetEvent evnt;
  while (enet_host_service(host_, &evnt, 0) > 0) {
    net::Peer *peer = static_cast<net::Peer *>(evnt.peer->data);

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

fw::Status Host::try_listen(int port) {
  ENetAddress addr;
  addr.host = ENET_HOST_ANY;
  addr.port = static_cast<enet_uint16>(port);

  // 32 connections allowed, unlimited bandwidth
  host_ = enet_host_create(port == 0 ? 0 : &addr, 32, ENET_PROTOCOL_MAXIMUM_CHANNEL_COUNT, 0, 0);
  if (host_ == nullptr) {
    fw::debug << "error listening on port " << port << std::endl;
    return fw::ErrorStatus(absl::StrCat("error listening on port ", port));
  }

  return fw::OkStatus();
}

void Host::on_peer_connect(ENetPeer *peer) {
  fw::debug << "new connection received from " << peer->address.host << ":" << peer->address.port
            << std::endl;

  auto new_peer = std::make_shared<Peer>(this, peer, true);
  new_connections_.push_back(new_peer);
  peer->data = new_peer.get();
}

std::vector<std::shared_ptr<Peer>> Host::get_new_connections() {
  std::vector<std::shared_ptr<Peer>> new_conns;
  std::swap(new_conns, new_connections_);

  return new_conns;
}

}
