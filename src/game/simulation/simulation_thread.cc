#include <game/simulation/simulation_thread.h>

#include <memory>
#include <thread>

#include <framework/logging.h>
#include <framework/lua.h>
#include <framework/net.h>
#include <framework/settings.h>
#include <framework/status.h>
#include <framework/timer.h>

#include <game/simulation/player.h>
#include <game/simulation/remote_player.h>
#include <game/simulation/local_player.h>
#include <game/simulation/commands.h>
#include <game/ai/ai_player.h>

namespace game {

SimulationThread *SimulationThread::instance = new SimulationThread();

SimulationThread::SimulationThread() :
    turn_(0), game_id_(0), stopped_(false) {
}

void SimulationThread::initialize() {
  // create the local_player that represents us.
  local_player_ = std::make_shared<LocalPlayer>();
  players_.push_back(local_player_);

  host_ = std::make_shared<fw::net::Host>();
  thread_ = std::thread(std::bind(&SimulationThread::thread_proc, this));
}

void SimulationThread::destroy() {
  stopped_ = true;
  stopped_cond_.notify_all();
  thread_.join();
}

fw::Status SimulationThread::connect(uint64_t game_id, std::string address, uint8_t player_no) {
  // remember the identifier of the game we're joining
  game_id_ = game_id;

  // our player_no is what we got from the session server when we asked to
  // join the game.
  local_player_->set_player_no(player_no);

  // create a new remote_player for the Host player and connect to it
  ASSIGN_OR_RETURN(auto player, RemotePlayer::connect(host_, address));
  players_.push_back(player);
  return fw::OkStatus();
}

fw::Status SimulationThread::connect_player(std::string address) {
  ASSIGN_OR_RETURN(auto player, RemotePlayer::connect(host_, address));
  players_.push_back(player);
  return fw::OkStatus();
}

void SimulationThread::new_game(uint64_t game_id) {
  // remember the identifier of the game.
  game_id_ = game_id;
}

int SimulationThread::get_listen_port() const {
  return host_->get_listen_port();
}

void SimulationThread::set_map_name(std::string const &value) {
  if (map_name_ == value)
    return;

  map_name_ = value;
  // todo: update players of the new map
}

void SimulationThread::send_chat_msg(std::string const &msg) {
  for(auto &player : players_) {
    player->send_chat_msg(msg);
  }
}

std::shared_ptr<Player> SimulationThread::get_player(uint8_t player_no) const {
  for (auto &player : players_) {
    if (player->get_player_no() == player_no) {
      return player;
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

  for (auto &player : players_) {
    player->post_commands(posted_commands_);
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

void SimulationThread::add_ai_player(std::shared_ptr<AIPlayer> const &player) {
  players_.push_back(player);
  sig_players_changed.Emit();
}

/** This is the thread procedure for running the simulation thread. */
void SimulationThread::thread_proc() {
  auto status = host_->listen(fw::Settings::get<std::string> ("listen-port"));
  if (!status.ok()) {
    LOG(ERR) << "error listening: " << status;
    // TODO: terminate, or something?
    return;
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
    std::vector<std::shared_ptr<fw::net::Peer>> new_connections = host_->get_new_connections();
    for (auto &new_peer : new_connections) {
      players_.push_back(std::make_shared<RemotePlayer>(host_, new_peer, true));
      sig_players_changed.Emit();
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
    for (auto &player : players_) {
      player->update();
    }

    std::unique_lock<std::mutex> lock(mutex);
    stopped_cond_.wait_until(lock, start + std::chrono::milliseconds(200));
  }
}

}
