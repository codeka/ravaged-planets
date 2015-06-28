
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#include <framework/logging.h>
#include <framework/net.h>
#include <framework/settings.h>
#include <framework/exception.h>

#include <game/simulation/simulation_thread.h>
#include <game/simulation/player.h>
#include <game/simulation/remote_player.h>
#include <game/simulation/local_player.h>
#include <game/simulation/commands.h>
#include <game/ai/ai_player.h>

namespace game {

simulation_thread *simulation_thread::instance = new simulation_thread();

simulation_thread::simulation_thread() :
    _host(nullptr), _thread(nullptr), _turn(0), _game_id(0), _local_player(nullptr) {
}

simulation_thread::~simulation_thread() {
  delete _host;
  delete _thread;
  delete _local_player;
}

void simulation_thread::initialize() {
  // create the local_player that represents us.
  _local_player = new local_player();
  _players.push_back(_local_player);

  _host = new fw::net::host();
  _thread = new boost::thread(boost::bind(&simulation_thread::thread_proc, this));
}

void simulation_thread::destroy() {
  _thread->interrupt();
  _thread->join();
}

void simulation_thread::connect(uint64_t game_id, std::string address, uint8_t player_no) {
  // remember the identifier of the game we're joining
  _game_id = game_id;

  // our player_no is what we got from the session server when we asked to
  // join the game.
  _local_player->set_player_no(player_no);

  // create a new remote_player for the host player and connect to it
  remote_player *player = remote_player::connect(_host, address);
  _players.push_back(player);
}

void simulation_thread::connect_player(std::string address) {
  remote_player *player = remote_player::connect(_host, address);
  _players.push_back(player);
}

void simulation_thread::new_game(uint64_t game_id) {
  // remember the identifier of the game.
  _game_id = game_id;
}

int simulation_thread::get_listen_port() const {
  return _host->get_listen_port();
}

void simulation_thread::set_map_name(std::string const &value) {
  if (_map_name == value)
    return;

  _map_name = value;
  // todo: update players of the new map
}

void simulation_thread::send_chat_msg(std::string const &msg) {
  BOOST_FOREACH(player *plyr, _players) {
    plyr->send_chat_msg(msg);
  }
}

player *simulation_thread::get_player(uint8_t player_no) const {
  BOOST_FOREACH(player *plyr, _players) {
    if (plyr->get_player_no() == player_no) {
      return plyr;
    }
  }

  return nullptr;
}

void simulation_thread::post_command(std::shared_ptr<command> &cmd) {
  _posted_commands.push_back(cmd);
}

void simulation_thread::enqueue_posted_commands() {
  BOOST_FOREACH(std::shared_ptr<command> &cmd, _posted_commands) {
    enqueue_command(cmd);
  }

  BOOST_FOREACH(player *p, _players) {
    p->post_commands(_posted_commands);
  }

  _posted_commands.clear();
}

void simulation_thread::enqueue_command(std::shared_ptr<command> &cmd) {
  turn_id turn = _turn + 1;

  command_queue::iterator it = _commands.find(turn);
  if (it == _commands.end()) {
    _commands[turn] = command_queue::mapped_type();
    it = _commands.find(turn);
  }

  command_queue::mapped_type &command_list = it->second;
  command_list.push_back(cmd);
}

void simulation_thread::add_ai_player(ai_player *plyr) {
  _players.push_back(plyr);
  sig_players_changed();
}

/** This is the thread procedure for running the simulation thread. */
void simulation_thread::thread_proc() {
  fw::settings stg;
  if (!_host->listen(stg.get_value<std::string> ("listen-port"))) {
    BOOST_THROW_EXCEPTION(fw::exception()
        << fw::message_error_info("could not listen on port(s): " + stg.get_value<std::string>("listen-port")));
  }

  for (;;) {
    boost::posix_time::ptime start(boost::get_system_time());
    _host->update();
    _turn++;

    // at the start of each turn, we post the commands for the *next* turn
    enqueue_posted_commands();

    // next, check for any new connections that the host has detected for us, this shouldn't happen
    // once the game is underway, but you never know (in that case, we need to reject them!)
    std::vector<fw::net::peer *> new_connections = _host->get_new_connections();
    BOOST_FOREACH(fw::net::peer *new_peer, new_connections) {
      _players.push_back(new remote_player(_host, new_peer, true));
      sig_players_changed();
    }

    // execute all of the commands that are due this turn
    command_queue::iterator it = _commands.find(_turn);
    if (it != _commands.end()) {
      command_queue::mapped_type &command_list = it->second;
      BOOST_FOREACH(std::shared_ptr<command> &cmd, command_list) {
        cmd->execute();
      }

      // we'll not need this turn again...
      _commands.erase(it);
    }

    // finally, update each player.
    BOOST_FOREACH(player *plyr, _players) {
      plyr->update();
    }

    try {
      // we want to make sure each "iteration" is 200 milliseconds, so it's actually
      // 200ms minus the time taken for the above code
      boost::thread::sleep(start + boost::posix_time::millisec(200));
    } catch (boost::thread_interrupted const &) {
      // we'll get a boost::thread_interrupted when it's time to quit
      fw::debug << "simulation thread got stop signal, shutting down." << std::endl;
      return;
    }
  }
}

}
