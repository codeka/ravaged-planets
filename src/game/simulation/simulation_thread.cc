
#include <thread>

#include <framework/logging.h>
#include <framework/lua.h>
#include <framework/net.h>
#include <framework/settings.h>
#include <framework/exception.h>
#include <framework/timer.h>

#include <game/simulation/simulation_thread.h>
#include <game/simulation/player.h>
#include <game/simulation/remote_player.h>
#include <game/simulation/local_player.h>
#include <game/simulation/commands.h>
#include <game/ai/ai_player.h>

namespace game {

SimulationThread *SimulationThread::instance = new SimulationThread();

SimulationThread::SimulationThread() :
    host_(nullptr), turn_(0), game_id_(0), local_player_(nullptr), stopped_(false) {
}

SimulationThread::~SimulationThread() {
  delete host_;
  delete local_player_;
}

void SimulationThread::initialize() {
  // create the local_player that represents us.
  local_player_ = new LocalPlayer();
  players_.push_back(local_player_);

  host_ = new fw::net::Host();
  thread_ = std::thread(std::bind(&SimulationThread::thread_proc, this));
}

void SimulationThread::destroy() {
  stopped_ = true;
  stopped_cond_.notify_all();
  thread_.join();
}

void SimulationThread::connect(uint64_t game_id, std::string address, uint8_t player_no) {
  // remember the identifier of the game we're joining
  game_id_ = game_id;

  // our player_no is what we got from the session server when we asked to
  // join the game.
  local_player_->set_player_no(player_no);

  // create a new remote_player for the Host player and connect to it
  RemotePlayer *player = RemotePlayer::connect(host_, address);
  players_.push_back(player);
}

void SimulationThread::connect_player(std::string address) {
  RemotePlayer *player = RemotePlayer::connect(host_, address);
  players_.push_back(player);
}

void SimulationThread::new_game(uint64_t game_id) {
  // remember the identifier of the game.
  game_id_ = game_id;
}

int SimulationThread::get_listen_port() const {
  return host_->get_listen_port();
}

void SimulationThread::set_map_name(std::string const &ParticleRotation) {
  if (map_name_ == ParticleRotation)
    return;

  map_name_ = ParticleRotation;
  // todo: update players of the new map
}

void SimulationThread::send_chat_msg(std::string const &msg) {
  for(Player *plyr : players_) {
    plyr->send_chat_msg(msg);
  }
}

Player *SimulationThread::get_player(uint8_t player_no) const {
  for (Player *plyr : players_) {
    if (plyr->get_player_no() == player_no) {
      return plyr;
    }
  }

  return nullptr;
}

void SimulationThread::post_command(std::shared_ptr<Command> &cmd) {
  posted_commands_.push_back(cmd);
}

void SimulationThread::enqueue_posted_commands() {
  for (std::shared_ptr<Command> &cmd : posted_commands_) {
    enqueue_command(cmd);
  }

  for (Player *p : players_) {
    p->post_commands(posted_commands_);
  }

  posted_commands_.clear();
}

void SimulationThread::enqueue_command(std::shared_ptr<Command> &cmd) {
  turn_id turn = turn_ + 1;

  auto it = commands_.find(turn);
  if (it == commands_.end()) {
    commands_[turn] = std::vector<std::shared_ptr<Command>>();
    it = commands_.find(turn);
  }

  auto &command_list = it->second;
  command_list.push_back(cmd);
}

void SimulationThread::add_ai_player(AIPlayer *plyr) {
  players_.push_back(plyr);
  sig_players_changed();
}

/** This is the thread procedure for running the simulation thread. */
void SimulationThread::thread_proc() {
  fw::Settings stg;
  if (!host_->listen(stg.get_value<std::string> ("listen-port"))) {
    BOOST_THROW_EXCEPTION(fw::Exception()
        << fw::message_error_info("could not listen on port(s): " + stg.get_value<std::string>("listen-port")));
  }

  std::mutex mutex;

  while (!stopped_) {
    fw::Clock::time_point start(fw::Clock::now());
    host_->update();
    turn_++;

    // at the start of each turn, we post the commands for the *next* turn
    enqueue_posted_commands();

    // next, check for any new connections that the Host has detected for us, this shouldn't happen
    // once the game is underway, but you never know (in that case, we need to reject them!)
    std::vector<fw::net::Peer *> new_connections = host_->get_new_connections();
    for (fw::net::Peer *new_peer : new_connections) {
      players_.push_back(new RemotePlayer(host_, new_peer, true));
      sig_players_changed();
    }

    // execute all of the commands that are due this turn
    auto it = commands_.find(turn_);
    if (it != commands_.end()) {
      auto &command_list = it->second;
      for (std::shared_ptr<Command> &cmd : command_list) {
        cmd->execute();
      }

      // we'll not need this turn again...
      commands_.erase(it);
    }

    // finally, update each player.
    for (Player *plyr : players_) {
      plyr->update();
    }

    std::unique_lock<std::mutex> lock(mutex);
    stopped_cond_.wait_until(lock, start + std::chrono::milliseconds(200));
  }
}

}
