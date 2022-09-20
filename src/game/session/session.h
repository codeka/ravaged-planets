#pragma once

#include <memory>
#include <mutex>
#include <queue>

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

namespace fw {
class Http;
}

namespace game {
class session_request;
class remote_game;

class session {
public:
  // these are the different states the session can be in
  enum session_state {
    disconnected, in_error, logging_in, logged_in, logging_out, joining_lobby,
  };

private:
  static session *_instance;

  session_state _state;
  std::shared_ptr<session_request> _curr_req;
  std::queue<std::shared_ptr<session_request>> _pending;
  uint64_t _session_id;
  uint32_t _user_id;
  std::string _user_name;
  std::string _error_msg;

  std::mutex mutex_;

  // this is called by the login static function. you use that to actually log in
  // and create the session object
  session();

  std::string get_base_url() const;

  // adds a request to the queue
  void add_request(std::shared_ptr<session_request> req);
  void begin_request(std::shared_ptr<session_request> req);

public:
  static session *get_instance() {
    return _instance;
  }
  ~session();

  // this must be called every now and then (e.g. every frame) to update
  // the state, post and queued requests and so on
  void update();

  // logs you in to the server with the given username and password
  std::shared_ptr<session_request> login(std::string const &username, std::string const &password);

  // logs you out and "disconnects".
  std::shared_ptr<session_request> logout();

  // this is called when you check the "enable multiplayer" checkbox, we need to create
  // a new game for people to join
  std::shared_ptr<session_request> create_game();

  // joins the game with the given lobby identifier
  std::shared_ptr<session_request> join_game(uint64_t lobby_id);

  // gets the list of remote games that we can connect to. when the list is refreshed (may
  // take some time) we'll call the given callback with the list
  std::shared_ptr<session_request> get_games_list(std::function<void(std::vector<remote_game> const &)> callback);

  // confirm that the given player has joined this game
  std::shared_ptr<session_request> confirm_player(uint64_t game_id, uint32_t user_id);

  // gets the current state (if a post is in progress, we'll check if it's finished
  // and parse the response at the same time)
  session_state get_state() {
    return _state;
  }
  void set_state(session_state state);

  // this signal is fired when the session state changes.
  boost::signals2::signal<void(session_state)> sig_state_changed;

  // gets and sets our session_id and user_name
  uint64_t get_session_id() const {
    return _session_id;
  }
  uint32_t get_user_id() const {
    return _user_id;
  }
  std::string get_user_name() const {
    return _state == logged_in ? _user_name : "";
  }

  // if the state is session::in_error, this'll get the corresponding error message
  std::string const &get_error_msg() const {
    return _error_msg;
  }
};

}
