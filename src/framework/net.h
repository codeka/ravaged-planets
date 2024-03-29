#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <boost/noncopyable.hpp>
#include <enet/enet.h>

#include <framework/packet.h>

namespace fw {
namespace net {
class Peer;

// The Framework automatically calls fw::net::initialize() and fw::net::destroy() to ensure we're all ready to go.
void initialize();
void destroy();

class Host {
private:
  ENetHost *host_;
  int _listen_port;
  std::vector<Peer *> _new_connections;

  bool try_listen(int port);
  virtual void on_peer_connect(ENetPeer *Peer);

public:
  Host();
  virtual ~Host();

  Host(const Host&) = delete;
  Host& operator=(const Host&) = delete;

  bool listen(std::string port_range);
  bool listen(int min_port, int max_port);

  /** Connects to the given address and returns a new Peer representing that connection. */
  Peer *connect(std::string address);

  /** This should be called frequently (e.g. every frame) to ensure we're up to date. */
  virtual void update();

  /** Gets a vector of all the new connections that we've had since your last call to get_new_connections()/ */
  std::vector<Peer *> get_new_connections();

  int get_listen_port() const {
    return _listen_port;
  }
};

/** This is the base "Peer" class that represents a connection to one of our peers in the game. */
class Peer {
protected:
  friend class Host;

  Host *host_;
  ENetPeer *peer_;
  bool connected_;

  std::function<void(std::shared_ptr<Packet> const &)> handler_;

  virtual void on_connect();
  virtual void on_receive(ENetPacket *Packet, enet_uint8 channel);
  virtual void on_disconnect();

  Peer(Host *hst, ENetPeer *Peer, bool connected);
  Peer(const Peer&) = delete;
  Peer& operator=(const Peer&) = delete;
public:
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
};
}
}
