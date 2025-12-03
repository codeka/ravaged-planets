#pragma once

#include <memory>
#include <mutex>
#include <queue>

#include <framework/signals.h>

namespace fw {
class Http;
}

namespace game {
class SessionRequest;
class RemoteGame;

class Session {
public:
  // these are the different states the session can be in
  enum SessionState {
    kDisconnected, kInError, kLoggingIn, kLoggedIn, kLoggingOut, kJoiningLobby,
  };

private:
  static Session *instance_;

  SessionState state_;
  std::shared_ptr<SessionRequest> curr_req_;
  std::queue<std::shared_ptr<SessionRequest>> pending_;
  uint64_t session_id_;
  uint32_t user_id_;
  std::string user_name_;

  std::mutex mutex_;

  // this is called by the login static function. you use that to actually log in
  // and create the session object
  Session();

  std::string get_base_url() const;

  // adds a request to the queue
  void add_request(std::shared_ptr<SessionRequest> req);
  void begin_request(std::shared_ptr<SessionRequest> req);

public:
  static Session *get_instance() {
    return instance_;
  }
  ~Session();

  // this must be called every now and then (e.g. every frame) to update
  // the state, post and queued requests and so on
  void update();

  // logs you in to the server with the given username and password
  std::shared_ptr<SessionRequest> login(std::string const &username, std::string const &password);

  // logs you out and "disconnects".
  std::shared_ptr<SessionRequest> logout();

  // this is called when you check the "enable multiplayer" checkbox, we need to create
  // a new game for people to join
  std::shared_ptr<SessionRequest> create_game();

  // joins the game with the given lobby identifier
  std::shared_ptr<SessionRequest> join_game(uint64_t lobby_id);

  // gets the list of remote games that we can connect to. when the list is refreshed (may
  // take some time) we'll call the given callback with the list
  std::shared_ptr<SessionRequest> get_games_list(std::function<void(std::vector<RemoteGame> const &)> callback);

  // confirm that the given player has joined this game
  std::shared_ptr<SessionRequest> confirm_player(uint64_t game_id, uint32_t user_id);

  // gets the current state (if a post is in progress, we'll check if it's finished
  // and parse the response at the same time)
  SessionState get_state() {
    return state_;
  }
  void set_state(SessionState state);

  // this signal is fired when the session state changes.
  fw::Signal<SessionState> sig_state_changed;

  // gets and sets our session_id and user_name
  uint64_t get_session_id() const {
    return session_id_;
  }
  uint32_t get_user_id() const {
    return user_id_;
  }
  std::string get_user_name() const {
    return state_ == kLoggedIn ? user_name_ : "";
  }
};

}
