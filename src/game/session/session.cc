#include <framework/exception.h>
#include <framework/settings.h>
#include <framework/http.h>
#include <framework/logging.h>
#include <framework/xml.h>

#include <game/session/session.h>
#include <game/session/session_request.h>

namespace game {

session *session::_instance = new session();

session::session() :
    _user_id(0), session_id_(0), _state(session::disconnected) {
}

session::~session() {
}

// logs the user in and returns a new session_request object. The session will be
// in the "logging_in" state until the server responds.
std::shared_ptr<SessionRequest> session::login(std::string const &username, std::string const &password) {
  _user_name = username;
  std::shared_ptr<SessionRequest> req(new login_session_request(username, password));
  add_request(req);
  return req;
}

// logs the user out and removes their session information
std::shared_ptr<SessionRequest> session::logout() {
  std::shared_ptr<SessionRequest> req(new logout_session_request());
  add_request(req);
  return req;
}

// registers a new lobby with the server, people will be able to see it and join if they so choose.
std::shared_ptr<SessionRequest> session::create_game() {
  std::shared_ptr<SessionRequest> req(new create_game_session_request());
  add_request(req);
  return req;
}

// joins the game with the given lobby_id
std::shared_ptr<SessionRequest> session::join_game(uint64_t lobby_id) {
  std::shared_ptr<SessionRequest> req(new join_game_session_request(lobby_id));
  add_request(req);
  return req;
}

// refreshes the games list by querying the server
std::shared_ptr<SessionRequest> session::get_games_list(
    std::function<void(std::vector<remote_game> const &)> callback) {
  std::shared_ptr<SessionRequest> req(new list_games_session_request(callback));
  add_request(req);
  return req;
}

// requests that the server "confirms" that the given session_id is a valid one.
std::shared_ptr<SessionRequest> session::confirm_player(uint64_t game_id, uint32_t user_id) {
  std::shared_ptr<SessionRequest> req(new confirm_player_session_request(game_id, user_id));
  add_request(req);
  return req;
}

// adds a request to the queue (if the queue is empty, we'll post the request straight
// away)
void session::add_request(std::shared_ptr<SessionRequest> req) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (!_curr_req) {
    begin_request(req);
    return;
  }

  _pending.push(req);
}

void session::begin_request(std::shared_ptr<SessionRequest> req) {
  _curr_req = req;

  req->set_session_id(session_id_);
  req->set_user_id(_user_id);
  req->begin(get_base_url());
}

// this is called every frame or so to update our state
void session::update() {
  std::unique_lock<std::mutex> lock(mutex_);

  if (!_curr_req)
    return;

  SessionRequest::update_result res = _curr_req->update();
  if (res == SessionRequest::still_going)
    return;

  session_id_ = _curr_req->get_session_id();
  _user_id = _curr_req->get_user_id();
  _error_msg = _curr_req->get_error_msg();

  if (res == SessionRequest::in_error) {
    set_state(session::in_error);
  }

  _curr_req.reset();
  if (_pending.size() > 0) {
    std::shared_ptr<SessionRequest> req = _pending.front();
    _pending.pop();

    begin_request(req);
  }
}

void session::set_state(session_state state) {
  _state = state;

  // fire the signal so any listeners know
  sig_state_changed(_state);
}

std::string session::get_base_url() const {
  fw::Settings stg;
  std::string base_url = stg.get_value<std::string>("server-url");
  if (base_url[base_url.length() - 1] != '/') {
    base_url += "/";
  }
  return base_url;
}

}
