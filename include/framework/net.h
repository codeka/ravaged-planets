#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <boost/noncopyable.hpp>
#include <enet/enet.h>

namespace fw {
namespace net {
class packet;
class peer;

// The framework automatically calls fw::net::initialize() and fw::net::destroy() to ensure we're all ready to go.
void initialize();
void destroy();

class host: public boost::noncopyable {
private:
  ENetHost *_host;
  int _listen_port;
  std::vector<peer *> _new_connections;

  bool try_listen(int port);
  virtual void on_peer_connect(ENetPeer *peer);

public:
  host();
  virtual ~host();

  bool listen(std::string port_range);
  bool listen(int min_port, int max_port);

  /** Connects to the given address and returns a new peer representing that connection. */
  peer *connect(std::string address);

  /** This should be called frequently (e.g. every frame) to ensure we're up to date. */
  virtual void update();

  /** Gets a vector of all the new connections that we've had since your last call to get_new_connections()/ */
  std::vector<peer *> get_new_connections();

  int get_listen_port() const {
    return _listen_port;
  }
};

/** This is the base "peer" class that represents a connection to one of our peers in the game. */
class peer: public boost::noncopyable {
protected:
  friend class host;

  host *_host;
  ENetPeer *_peer;
  bool _connected;

  std::function<void(std::shared_ptr<packet> const &)> _handler;

  virtual void on_connect();
  virtual void on_receive(ENetPacket *packet, enet_uint8 channel);
  virtual void on_disconnect();

  peer(host *hst, ENetPeer *peer, bool connected);
public:
  virtual ~peer();

  /** Sets the handler we'll call when a packet comes in from this peer. */
  void set_handler(std::function<void(std::shared_ptr<packet> const &)> handler) {
    _handler = handler;
  }

  /** Sends the given packet to this peer. */
  void send(packet &pkt, int channel = 0);

  bool is_connected() const {
    return _connected;
  }
};
}
}
