#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <boost/noncopyable.hpp>
#include <enet/enet.h>

#include <framework/packet.h>
#include <framework/status.h>

namespace fw {
namespace net {
class Peer;

// The Framework automatically calls fw::net::initialize() and fw::net::destroy() to ensure we're
// all ready to go.
fw::Status initialize();
void destroy();

class Host {
public:
  Host();
  virtual ~Host();

  Host(const Host&) = delete;
  Host& operator=(const Host&) = delete;

  fw::Status listen(std::string port_range);
  fw::Status listen(int min_port, int max_port);

  /** Connects to the given address and returns a new Peer representing that connection. */
  fw::StatusOr<std::shared_ptr<Peer>> connect(std::string address);

  /** This should be called frequently (e.g. every frame) to ensure we're up to date. */
  virtual void update();

  /**
   * Gets a vector of all the new connections that we've had since your last call to
   * get_new_connections()/
   */
  std::vector<std::shared_ptr<Peer>> get_new_connections();

  int get_listen_port() const {
    return listen_port_;
  }

private:
  ENetHost *host_;
  int listen_port_;
  std::vector<std::shared_ptr<Peer>> new_connections_;

  fw::Status try_listen(int port);
  virtual void on_peer_connect(ENetPeer *enet_peer);
};

/** This is the base "Peer" class that represents a connection to one of our peers in the game. */
class Peer {
public:
  Peer(Host *host, ENetPeer *peer, bool connected);
  Peer(const Peer&) = delete;
  Peer& operator=(const Peer&) = delete;
  virtual ~Peer();

  /** Sets the handler we'll call when a Packet comes in from this Peer. */
  void set_handler(std::function<void(std::shared_ptr<Packet> const &)> handler) {
    handler_ = handler;
  }

  /** Sends the given Packet to this Peer. */
  void send(Packet &pkt, int channel = 0);

  bool is_connected() const {
    return connected_;
  }
protected:
  friend class Host;

  Host *host_;
  ENetPeer *peer_;
  bool connected_;

  std::function<void(std::shared_ptr<Packet> const &)> handler_;

  virtual void on_connect();
  virtual void on_receive(ENetPacket *Packet, enet_uint8 channel);
  virtual void on_disconnect();
};
}
}
