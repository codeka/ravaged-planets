#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <boost/signals2/signal.hpp>

namespace fw {
namespace net {
class host;
}
}

namespace game {
class player;
class ai_player;
class local_player;
class command;

typedef uint32_t turn_id;

/**
 * The main simulation for the game runs in a separate thread, and at a separate rate to the rendering, etc. This
 * class encapsulates the functions of the simulation thread.
 */
class simulation_thread {
public:
  // these are the functions we use when various events occur in the simulation.
  typedef std::function<void()> callback_fn;

private:
  static simulation_thread *instance;

  fw::net::host *_host;
  std::thread *_thread;
  local_player *_local_player;
  std::vector<player *> _players;
  std::string _map_name;
  uint64_t _game_id;
  turn_id _turn;

  typedef std::map<turn_id, std::vector<std::shared_ptr<command>>> command_queue;
  command_queue _commands;

  /**
   * This is the list of commands that the player posted to us in this turn. At the end of the current turn, we'll
   * enqueue it to the command queue and also notify other players of it.
   */
  std::vector<std::shared_ptr<command>> _posted_commands;

  /**
   * At the end of each turn, this is called to enqueue all the commands that were posted and notify other players
   * of them as well.
   */
  void enqueue_posted_commands();

  void thread_proc();
public:
  simulation_thread();
  ~simulation_thread();

  void initialize();
  void destroy();

  /** Connect to a remote simulation on another computer, the given player_no is our player# for the game. */
  void connect(uint64_t game_id, std::string address, uint8_t player_no);

  /**
   * Connect to another remote player who's already part of this simulation (this is called when we're joining
   * an existing game)
   */
  void connect_player(std::string address);

  /**
   * When a new multiplayer game is started, this is called to set up the game_id and that, once everything is ready
   * to go, call start_game()
   */
  void new_game(uint64_t game_id);

  void set_map_name(std::string const &value);
  std::string const &get_map_name() const {
    return _map_name;
  }

  /** This signal is fired when the list of players changes (e.g. someone joins, leaves, etc). */
  boost::signals2::signal<void()> sig_players_changed;

  /** This signal is fired when a chat message is received. */
  boost::signals2::signal<void(std::string const &, std::string const &)> sig_chat;

  /** Send a chat message from us to all of the players. */
  void send_chat_msg(std::string const &msg);

  /**
   * This is the called by the input subsystem/game world when the user initiates a command (e.g. the click on terrain
   * to order a unit to move, etc). we must post the command to our command queue and notify remote players of the
   * pending command.
   */
  void post_command(std::shared_ptr<command> &cmd);

  /**
   * This is just a convenience. normally, you'll have a shared_ptr to something that derives from command and this'll
   * just do a std::dynamic_pointer_cast for you.
   */
  template<typename T>
  inline void post_command(std::shared_ptr<T> &cmd) {
    std::shared_ptr<command> real_cmd(std::dynamic_pointer_cast<command>(cmd));
    post_command(real_cmd);
  }

  /** Enqueues a command for the next turn. this is called when we receive a command from other players. */
  void enqueue_command(std::shared_ptr<command> &cmd);

  /** Adds a new AI player to the list of players. */
  void add_ai_player(ai_player *plyr);

  /**
   * Gets an std::vector<> of the current players in the game. This is a copy of our vector, so don't call this
   * too often.
   */
  std::vector<player *> get_players() const {
    return _players;
  }

  /** Gets the local_player instance that represents the local player. */
  local_player *get_local_player() const {
    return _local_player;
  }

  /** Gets a pointer to the player with the given player_no. */
  player *get_player(uint8_t player_no) const;

  /** Gets the port number we're listening for peer connections on. */
  int get_listen_port() const;
  fw::net::host *get_host() const {
    return _host;
  }
  uint64_t get_game_id() const {
    return _game_id;
  }
  static simulation_thread *get_instance() {
    return instance;
  }
};

}
