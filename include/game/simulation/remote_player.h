#pragma once

#include "player.h"

namespace fw {
namespace net {
class peer;
class host;
class packet;
}
}

namespace game {
class session_request;

/** This class represents a remote player which is connected to us via the internet. */
class remote_player: public player {
private:
  fw::net::host *_host;
  fw::net::peer *_peer;
  bool _connected;

  /** This is called whenever we receive a packet from our peer. */
  void packet_handler(std::shared_ptr<fw::net::packet> const &pkt);

  // the following are the individual "handler" functions for each of the packet
  // types we handle. The return value is the respone packet we'll respond with.
  void pkt_join_req(std::shared_ptr<fw::net::packet> pkt);
  void pkt_join_resp(std::shared_ptr<fw::net::packet> pkt);
  void pkt_chat(std::shared_ptr<fw::net::packet> pkt);
  void pkt_start_game(std::shared_ptr<fw::net::packet> pkt);
  void pkt_command(std::shared_ptr<fw::net::packet> pkt);

  // when we get a pkt_join, we ask the server to confirm the user. when the server
  // gets back to us, it'll call this method and we can respond to the original
  // player who just joined.
  void join_complete(session_request &req);

  // when we get a pkt_join, we'll also get a list of all the other player(s) in the
  // game. We'll need to connect to them as well. This is called when we've confirmed
  // the other player and it's time to actually connect to them
  void new_player_confirmed(session_request &req);

  // when we get a response to our pkt_join, we ask the server to confirm the host of
  // the game. when it's done that, we'll call this to set up the user_name and such
  void connect_complete(session_request &req);

public:
  remote_player(fw::net::host *host, fw::net::peer *peer, bool connected);
  virtual ~remote_player();

  // connect to the specified remote player
  static remote_player *connect(fw::net::host *host, std::string address);

  // this is called when our local player is ready to start the game, we should notify
  // our peer that we're ready.
  virtual void local_player_is_ready();

  // posts the given commands to this player for the next turn
  virtual void post_commands(std::vector<std::shared_ptr<command> > &commands);

  virtual void update();
  virtual void send_chat_msg(std::string const &msg);
};

}
