#include <functional>
#include <memory>
#include <boost/foreach.hpp>

#include <framework/net.h>
#include <framework/logging.h>
#include <framework/misc.h>

#include <game/session/session.h>
#include <game/session/session_request.h>
#include <game/simulation/player.h>
#include <game/simulation/local_player.h>
#include <game/simulation/remote_player.h>
#include <game/simulation/packets.h>
#include <game/simulation/simulation_thread.h>
#include <game/simulation/commands.h>

using namespace std::placeholders;

namespace game {

remote_player::remote_player(fw::net::host *host, fw::net::peer *peer, bool connected) :
    _host(host), _peer(peer), _connected(connected) {
  peer->set_handler(std::bind(&remote_player::packet_handler, this, _1));

  // give them a temporary username until the connection process has completed.
  _user_name = "Joining...";
}

remote_player::~remote_player() {
}

// connect to the specified remote player
remote_player *remote_player::connect(fw::net::host *host, std::string address) {
  fw::net::peer *peer = host->connect(address);
  return new remote_player(host, peer, false);
}

void remote_player::send_chat_msg(std::string const &msg) {
  chat_packet pkt;
  pkt.set_msg(msg);
  _peer->send(pkt);
}

// we need to send a packet to our peer and let them know we're ready to go
void remote_player::local_player_is_ready() {
  start_game_packet pkt;
  _peer->send(pkt);
}

void remote_player::post_commands(std::vector<std::shared_ptr<command>> &commands) {
  command_packet pkt;
  pkt.set_commands(commands);
  _peer->send(pkt);
}

// this is called whenever we receive a packet from our peer, we need to work out
// what kind of packet it is and hand it off to the correct handler function.
void remote_player::packet_handler(std::shared_ptr<fw::net::packet> const &pkt) {
  switch (pkt->get_identifier()) {
  case join_request_packet::identifier:
    pkt_join_req(pkt);
    break;
  case join_response_packet::identifier:
    pkt_join_resp(pkt);
    break;
  case chat_packet::identifier:
    pkt_chat(pkt);
    break;
  case start_game_packet::identifier:
    pkt_start_game(pkt);
    break;
  case command_packet::identifier:
    pkt_command(pkt);
    break;

  default:
    fw::debug << "  warning: unknown packet type: " << pkt->get_identifier() << std::endl;
    // TODO: response_packet = error_response_packet();
    break;
  }
}

// this is called when the remote player first connects to us, it's the "join" request.
void remote_player::pkt_join_req(std::shared_ptr<fw::net::packet> pkt) {
  std::shared_ptr<join_request_packet> req(std::dynamic_pointer_cast<join_request_packet>(pkt));
  _user_id = req->get_user_id();
  _colour = req->get_colour();

  // call the session and confirm the fact that this player is valid and that.
  std::shared_ptr<game::session_request> sess_req = session::get_instance()->confirm_player(
      simulation_thread::get_instance()->get_game_id(), _user_id);
  sess_req->set_complete_handler(std::bind(&remote_player::join_complete, this, _1));

  simulation_thread::get_instance()->sig_players_changed();
}

void remote_player::pkt_join_resp(std::shared_ptr<fw::net::packet> pkt) {
  std::shared_ptr<join_response_packet> resp(std::dynamic_pointer_cast<join_response_packet>(pkt));

  fw::debug << boost::format("connected to host, map is: %1%") % resp->get_map_name() << std::endl;
  BOOST_FOREACH(uint32_t other_user_id, resp->get_other_users()) {
    // the colour that we sent will be echo'd back to us, usually
    _colour = resp->get_my_colour();

    // we assume the first player is the host we're connect to, is that valid?
    if (_user_id == 0) {
      _user_id = other_user_id;

      // "your" colour is the colour we allow this "local" player to have
      fw::colour your_colour = resp->get_your_colour();
      simulation_thread::get_instance()->get_local_player()->set_colour(your_colour);

      // call the session and confirm the fact that this player is valid and that.
      std::shared_ptr<game::session_request> sess_req = session::get_instance()->confirm_player(
          simulation_thread::get_instance()->get_game_id(), _user_id);
      sess_req->set_complete_handler(std::bind(&remote_player::connect_complete, this, _1));
    } else {
      // it's not the host we just connected to, so we'll have toconnect to them as well!
      // but first, check whether we've already connected to them
      bool need_connect = true;
      if (other_user_id != 0) { // (it'll be zero if this is an AI player)
        BOOST_FOREACH(player *plyr, simulation_thread::get_instance()->get_players()) {
          if (plyr->get_user_id() == other_user_id) {
            fw::debug
                << boost::format("Already connected to player with user_id #%1%, not connecting again")
                % static_cast<int>(other_user_id) << std::endl;
            need_connect = false;
            break;
          }
        }
      }

      if (need_connect) {
        // call the session and confirm the fact that this player is valid and that, then connect to them
        std::shared_ptr<game::session_request> sess_req = session::get_instance()->confirm_player(
            simulation_thread::get_instance()->get_game_id(), other_user_id);
        sess_req->set_complete_handler(std::bind(&remote_player::new_player_confirmed, this, _1));
      }
    }
  }

  simulation_thread::get_instance()->sig_players_changed();
}

// when we get a chat packet, just fire the simulation_thread's chat_msg event
void remote_player::pkt_chat(std::shared_ptr<fw::net::packet> pkt) {
  std::shared_ptr<chat_packet> chat_pkt(std::dynamic_pointer_cast<chat_packet>(pkt));
  simulation_thread::get_instance()->sig_chat(_user_name, chat_pkt->get_msg());
}

// this is sent to us when our peer is ready to start the game
void remote_player::pkt_start_game(std::shared_ptr<fw::net::packet> pkt) {
  // mark this player as ready to start
  _is_ready_to_start = true;
}

// this is sent to us at the beginning of the turn, we need to enqueue the commands
// for the next turn.
void remote_player::pkt_command(std::shared_ptr<fw::net::packet> pkt) {
  std::shared_ptr<command_packet> command_pkt(std::dynamic_pointer_cast<command_packet>(pkt));
  BOOST_FOREACH(std::shared_ptr<command> &cmd, command_pkt->get_commands()) {
    fw::debug << "got command, id: " << static_cast<int>(cmd->get_identifier()) << std::endl;
    simulation_thread::get_instance()->enqueue_command(cmd);
  }
}

// checks whether the given colour is already taken (ignoring the given player)
bool colour_already_taken(player *except_for, fw::colour &col) {
  BOOST_FOREACH(player *plyr, simulation_thread::get_instance()->get_players()) {
    if (plyr != except_for && plyr->get_colour() == col) {
      return true;
    }
  }

  return false;
}

// when we get a pkt_join, we ask the server to confirm the user. when the server
// gets back to us, it'll call this method and we can respond to the original
// player who just joined.
void remote_player::join_complete(session_request &req) {
  confirm_player_session_request &cpsr = dynamic_cast<confirm_player_session_request &>(req);
  fw::debug << boost::format("remote_player::join_complete() addr=%1% username=%2%")
      % cpsr.get_address() % cpsr.get_user_name() << std::endl;

  // get the confirmed information about this new player
  _user_name = cpsr.get_user_name();
  _player_no = cpsr.get_player_no();

  // work out a random colour they can have (which hasn't already been taken by another player)
  if (_colour == fw::colour(0, 0, 0) || colour_already_taken(this, _colour)) {
    if (_colour == fw::colour(0, 0, 0)) {
      fw::debug << boost::format("remote player's colour hasn't been set yet, choosing one now") << std::endl;
    } else {
      fw::debug << boost::format("remote player's colour (%1%) is already taken, choosing another")
          % _colour << std::endl;
    }

    do {
      int colour_index = static_cast<int>(fw::random() * player_colours.size());
      _colour = player_colours[colour_index];
    } while (colour_already_taken(this, _colour));

    fw::debug << boost::format("changing remote player's colour to: %1%") % _colour << std::endl;
  }

  // once we get confirmation from the server, we can let the player know it's OK to join
  join_response_packet resp;
  resp.set_map_name(simulation_thread::get_instance()->get_map_name());
  resp.set_your_colour(_colour);
  resp.set_my_colour(simulation_thread::get_instance()->get_local_player()->get_colour());
  std::vector<player *> players = simulation_thread::get_instance()->get_players();
  BOOST_FOREACH(player *plyr, players) {
    if (plyr == this) {
      continue;
    }

    resp.get_other_users().push_back(plyr->get_user_id());
  }
  _peer->send(resp);

  simulation_thread::get_instance()->sig_players_changed();
}

void remote_player::connect_complete(session_request &req) {
  confirm_player_session_request &cpsr = dynamic_cast<confirm_player_session_request &>(req);
  fw::debug << boost::format("remote_player::connect_complete() host username=%1%") % cpsr.get_user_name() << std::endl;
  _user_name = cpsr.get_user_name();
  _player_no = cpsr.get_player_no();

  simulation_thread::get_instance()->sig_players_changed();
}

void remote_player::new_player_confirmed(session_request &req) {
  confirm_player_session_request &cpsr = dynamic_cast<confirm_player_session_request &>(req);
  fw::debug << boost::format("additional player (username: %1%) confirmed, connecting to them as well...")
      % cpsr.get_user_name() << std::endl;

  simulation_thread::get_instance()->connect_player(cpsr.get_address());
}

void remote_player::update() {
  if (!_connected && _peer->is_connected()) {
    fw::colour our_colour = simulation_thread::get_instance()->get_local_player()->get_colour();
    fw::debug << "connected to peer, sending \"hello\" packet! (our colour is: " << our_colour << ")" << std::endl;

    join_request_packet pkt;
    pkt.set_user_id(session::get_instance()->get_user_id());
    pkt.set_colour(our_colour);
    _peer->send(pkt);

    _connected = true;
  }
}

}
