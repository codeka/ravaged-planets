#include <functional>
#include <memory>

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

RemotePlayer::RemotePlayer(fw::net::Host *Host, fw::net::Peer *Peer, bool connected) :
    host_(Host), peer_(Peer), connected_(connected) {
  Peer->set_handler(std::bind(&RemotePlayer::packet_handler, this, _1));

  // give them a temporary username until the connection process has completed.
  user_name_ = "Joining...";
}

RemotePlayer::~RemotePlayer() {
}

// connect to the specified remote player
RemotePlayer *RemotePlayer::connect(fw::net::Host *Host, std::string address) {
  fw::net::Peer *Peer = Host->connect(address);
  return new RemotePlayer(Host, Peer, false);
}

void RemotePlayer::send_chat_msg(std::string const &msg) {
  ChatPacket pkt;
  pkt.set_msg(msg);
  peer_->send(pkt);
}

// we need to send a Packet to our Peer and let them know we're ready to go
void RemotePlayer::local_player_is_ready() {
  StartGamePacket pkt;
  peer_->send(pkt);
}

void RemotePlayer::post_commands(std::vector<std::shared_ptr<Command>> &commands) {
  CommandPacket pkt;
  pkt.set_commands(commands);
  peer_->send(pkt);
}

// this is called whenever we receive a Packet from our Peer, we need to work out
// what kind of Packet it is and hand it off to the correct handler function.
void RemotePlayer::packet_handler(std::shared_ptr<fw::net::Packet> const &pkt) {
  switch (pkt->get_identifier()) {
  case JoinRequestPacket::identifier:
    pkt_join_req(pkt);
    break;
  case JoinResponsePacket::identifier:
    pkt_join_resp(pkt);
    break;
  case ChatPacket::identifier:
    pkt_chat(pkt);
    break;
  case StartGamePacket::identifier:
    pkt_start_game(pkt);
    break;
  case CommandPacket::identifier:
    pkt_command(pkt);
    break;

  default:
    fw::debug << "  warning: unknown packet type: " << pkt->get_identifier() << std::endl;
    // TODO: response_packet = error_response_packet();
    break;
  }
}

// this is called when the remote player first connects to us, it's the "join" request.
void RemotePlayer::pkt_join_req(std::shared_ptr<fw::net::Packet> pkt) {
  std::shared_ptr<JoinRequestPacket> req(std::dynamic_pointer_cast<JoinRequestPacket>(pkt));
  user_id_ = req->get_user_id();
  color_ = req->get_color();

  // call the session and confirm the fact that this player is valid and that.
  std::shared_ptr<game::SessionRequest> sess_req = session::get_instance()->confirm_player(
      SimulationThread::get_instance()->get_game_id(), user_id_);
  sess_req->set_complete_handler(std::bind(&RemotePlayer::join_complete, this, _1));

  SimulationThread::get_instance()->sig_players_changed();
}

void RemotePlayer::pkt_join_resp(std::shared_ptr<fw::net::Packet> pkt) {
  std::shared_ptr<JoinResponsePacket> resp(std::dynamic_pointer_cast<JoinResponsePacket>(pkt));

  fw::debug << boost::format("connected to host, map is: %1%") % resp->get_map_name() << std::endl;
  for (uint32_t other_user_id : resp->get_other_users()) {
    // the color that we sent will be echo'd back to us, usually
    color_ = resp->get_my_color();

    // we assume the first player is the Host we're connect to, is that valid?
    if (user_id_ == 0) {
      user_id_ = other_user_id;

      // "your" color is the color we allow this "local" player to have
      fw::Color your_color = resp->get_your_color();
      SimulationThread::get_instance()->get_local_player()->set_color(your_color);

      // call the session and confirm the fact that this player is valid and that.
      std::shared_ptr<game::SessionRequest> sess_req = session::get_instance()->confirm_player(
          SimulationThread::get_instance()->get_game_id(), user_id_);
      sess_req->set_complete_handler(std::bind(&RemotePlayer::connect_complete, this, _1));
    } else {
      // it's not the Host we just connected to, so we'll have toconnect to them as well!
      // but first, check whether we've already connected to them
      bool need_connect = true;
      if (other_user_id != 0) { // (it'll be zero if this is an AI player)
        for (Player *plyr : SimulationThread::get_instance()->get_players()) {
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
        std::shared_ptr<game::SessionRequest> sess_req = session::get_instance()->confirm_player(
            SimulationThread::get_instance()->get_game_id(), other_user_id);
        sess_req->set_complete_handler(std::bind(&RemotePlayer::new_player_confirmed, this, _1));
      }
    }
  }

  SimulationThread::get_instance()->sig_players_changed();
}

// when we get a chat Packet, just fire the simulation_thread's chat_msg event
void RemotePlayer::pkt_chat(std::shared_ptr<fw::net::Packet> pkt) {
  std::shared_ptr<ChatPacket> chat_pkt(std::dynamic_pointer_cast<ChatPacket>(pkt));
  SimulationThread::get_instance()->sig_chat(user_name_, chat_pkt->get_msg());
}

// this is sent to us when our Peer is ready to start the game
void RemotePlayer::pkt_start_game(std::shared_ptr<fw::net::Packet> pkt) {
  // mark this player as ready to start
  is_ready_to_start_ = true;
}

// this is sent to us at the beginning of the turn, we need to enqueue the commands
// for the next turn.
void RemotePlayer::pkt_command(std::shared_ptr<fw::net::Packet> pkt) {
  std::shared_ptr<CommandPacket> command_pkt(std::dynamic_pointer_cast<CommandPacket>(pkt));
  for (std::shared_ptr<Command> &cmd : command_pkt->get_commands()) {
    fw::debug << "got command, id: " << static_cast<int>(cmd->get_identifier()) << std::endl;
    SimulationThread::get_instance()->enqueue_command(cmd);
  }
}

// checks whether the given color is already taken (ignoring the given player)
bool color_already_taken(Player *except_for, fw::Color &col) {
  for (Player *plyr : SimulationThread::get_instance()->get_players()) {
    if (plyr != except_for && plyr->get_color() == col) {
      return true;
    }
  }

  return false;
}

// when we get a pkt_join, we ask the server to confirm the user. when the server
// gets back to us, it'll call this method and we can respond to the original
// player who just joined.
void RemotePlayer::join_complete(SessionRequest &req) {
  confirm_player_session_request &cpsr = dynamic_cast<confirm_player_session_request &>(req);
  fw::debug << boost::format("remote_player::join_complete() addr=%1% username=%2%")
      % cpsr.get_address() % cpsr.get_user_name() << std::endl;

  // get the confirmed information about this new player
  user_name_ = cpsr.get_user_name();
  player_no_ = cpsr.get_player_no();

  // work out a Random color they can have (which hasn't already been taken by another player)
  if (color_ == fw::Color(0, 0, 0) || color_already_taken(this, color_)) {
    if (color_ == fw::Color(0, 0, 0)) {
      fw::debug << boost::format("remote player's color hasn't been set yet, choosing one now") << std::endl;
    } else {
      fw::debug << boost::format("remote player's color (%1%) is already taken, choosing another")
          % color_ << std::endl;
    }

    do {
      int color_index = static_cast<int>(fw::random() * player_colors.size());
      color_ = player_colors[color_index];
    } while (color_already_taken(this, color_));

    fw::debug << boost::format("changing remote player's color to: %1%") % color_ << std::endl;
  }

  // once we get confirmation from the server, we can let the player know it's OK to join
  JoinResponsePacket resp;
  resp.set_map_name(SimulationThread::get_instance()->get_map_name());
  resp.set_your_color(color_);
  resp.set_my_color(SimulationThread::get_instance()->get_local_player()->get_color());
  std::vector<Player *> players = SimulationThread::get_instance()->get_players();
  for (Player *plyr : players) {
    if (plyr == this) {
      continue;
    }

    resp.get_other_users().push_back(plyr->get_user_id());
  }
  peer_->send(resp);

  SimulationThread::get_instance()->sig_players_changed();
}

void RemotePlayer::connect_complete(SessionRequest &req) {
  confirm_player_session_request &cpsr = dynamic_cast<confirm_player_session_request &>(req);
  fw::debug << boost::format("remote_player::connect_complete() host username=%1%") % cpsr.get_user_name() << std::endl;
  user_name_ = cpsr.get_user_name();
  player_no_ = cpsr.get_player_no();

  SimulationThread::get_instance()->sig_players_changed();
}

void RemotePlayer::new_player_confirmed(SessionRequest &req) {
  confirm_player_session_request &cpsr = dynamic_cast<confirm_player_session_request &>(req);
  fw::debug << boost::format("additional player (username: %1%) confirmed, connecting to them as well...")
      % cpsr.get_user_name() << std::endl;

  SimulationThread::get_instance()->connect_player(cpsr.get_address());
}

void RemotePlayer::update() {
  if (!connected_ && peer_->is_connected()) {
    fw::Color our_color = SimulationThread::get_instance()->get_local_player()->get_color();
    fw::debug << "connected to peer, sending \"hello\" packet! (our color is: " << our_color << ")" << std::endl;

    JoinRequestPacket pkt;
    pkt.set_user_id(session::get_instance()->get_user_id());
    pkt.set_color(our_color);
    peer_->send(pkt);

    connected_ = true;
  }
}

}
