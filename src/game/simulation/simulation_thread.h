#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <thread>

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2/signal.hpp>

namespace fw {
namespace net {
class Host;
}
}

namespace game {
class Player;
class AIPlayer;
class LocalPlayer;
class Command;

typedef uint32_t turn_id;

/**
 * The main simulation for the game runs in a separate thread, and at a separate rate to the rendering, etc. This
 * class encapsulates the functions of the simulation thread.
 */
class SimulationThread {
public:
  // these are the functions we use when various events occur in the simulation.
  typedef std::function<void()> callback_fn;

private:
  static SimulationThread *instance;

  std::thread thread_;
  bool stopped_;
  std::condition_variable stopped_cond_;

  fw::net::Host *host_;
  LocalPlayer *local_player_;
  std::vector<Player *> players_;
  std::string map_name_;
  uint64_t game_id_;
  turn_id turn_;

  std::map<turn_id, std::vector<std::shared_ptr<Command>>> commands_;

  // This is the list of commands that the player posted to us in this turn. At the end of the current turn, we'll
  // enqueue it to the command queue and also notify other players of it.
  std::vector<std::shared_ptr<Command>> posted_commands_;

  // At the end of each turn, this is called to enqueue all the commands that were posted and notify other players
  // of them as well.
  void enqueue_posted_commands();

  void thread_proc();
public:
  SimulationThread();
  ~SimulationThread();

  void initialize();
  void destroy();

  // Connect to a remote simulation on another computer, the given player_no is our player# for the game.
  void connect(uint64_t game_id, std::string address, uint8_t player_no);

  // Connect to another remote player who's already part of this simulation (this is called when we're joining
  // an existing game)
  void connect_player(std::string address);

  // When a new multiplayer game is started, this is called to set up the game_id and that, once everything is ready
  // to go, call start_game()
  void new_game(uint64_t game_id);

  void set_map_name(std::string const &value);
  std::string const &get_map_name() const {
    return map_name_;
  }

  // This signal is fired when the list of players changes (e.g. someone joins, leaves, etc).
  boost::signals2::signal<void()> sig_players_changed;

  // This signal is fired when a chat message is received.
  boost::signals2::signal<void(std::string const &, std::string const &)> sig_chat;

  // Send a chat message from us to all of the players.
  void send_chat_msg(std::string const &msg);

  // This is the called by the Input subsystem/game world when the user initiates a command (e.g. the click on terrain
  // to order a unit to move, etc). we must post the command to our command queue and notify remote players of the
  // pending command.
  void post_command(std::shared_ptr<Command> &cmd);

  // This is just a convenience. normally, you'll have a shared_ptr to something that derives from command and this'll
  // just do a std::dynamic_pointer_cast for you.
  template<typename T>
  inline void post_command(std::shared_ptr<T> &cmd) {
    std::shared_ptr<Command> real_cmd(std::dynamic_pointer_cast<Command>(cmd));
    post_command(real_cmd);
  }

  // Enqueues a command for the next turn. this is called when we receive a command from other players.
  void enqueue_command(std::shared_ptr<Command> &cmd);

  // Adds a new AI player to the list of players.
  void add_ai_player(AIPlayer *plyr);

  // Gets an std::vector<> of the current players in the game. This is a copy of our vector, so don't call this
  // too often.
  std::vector<Player *> get_players() const {
    return players_;
  }

  /** Gets the local_player instance that represents the local player. */
  LocalPlayer *get_local_player() const {
    return local_player_;
  }

  /** Gets a pointer to the player with the given player_no. */
  Player *get_player(uint8_t player_no) const;

  /** Gets the port number we're listening for Peer connections on. */
  int get_listen_port() const;
  fw::net::Host *get_host() const {
    return host_;
  }
  uint64_t get_game_id() const {
    return game_id_;
  }
  static SimulationThread *get_instance() {
    return instance;
  }
};

}
