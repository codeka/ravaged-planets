
#include <framework/settings.h>
#include <framework/http.h>
#include <framework/logging.h>
#include <framework/xml.h>

#include <game/session/session.h>
#include <game/session/session_request.h>

namespace game {

Session *Session::instance_ = new Session();

Session::Session() :
    user_id_(0), session_id_(0), state_(SessionState::kDisconnected) {
}

Session::~Session() {
}

// logs the user in and returns a new session_request object. The session will be
// in the "logging_in" state until the server responds.
std::shared_ptr<SessionRequest> Session::login(std::string const &username, std::string const &password) {
  user_name_ = username;
  std::shared_ptr<SessionRequest> req(new LoginSessionRequest(username, password));
  add_request(req);
  return req;
}

// logs the user out and removes their session information
std::shared_ptr<SessionRequest> Session::logout() {
  std::shared_ptr<SessionRequest> req(new LogoutSessionRequest());
  add_request(req);
  return req;
}

// registers a new lobby with the server, people will be able to see it and join if they so choose.
std::shared_ptr<SessionRequest> Session::create_game() {
  std::shared_ptr<SessionRequest> req(new CreateGameSessionRequest());
  add_request(req);
  return req;
}

// joins the game with the given lobby_id
std::shared_ptr<SessionRequest> Session::join_game(uint64_t lobby_id) {
  std::shared_ptr<SessionRequest> req(new JoinGameSessionRequest(lobby_id));
  add_request(req);
  return req;
}

// refreshes the games list by querying the server
std::shared_ptr<SessionRequest> Session::get_games_list(
    std::function<void(std::vector<RemoteGame> const &)> callback) {
  std::shared_ptr<SessionRequest> req(new ListGamesSessionRequest(callback));
  add_request(req);
  return req;
}

// requests that the server "confirms" that the given session_id is a valid one.
std::shared_ptr<SessionRequest> Session::confirm_player(uint64_t game_id, uint32_t user_id) {
  std::shared_ptr<SessionRequest> req(new ConfirmPlayerSessionRequest(game_id, user_id));
  add_request(req);
  return req;
}

// adds a request to the queue (if the queue is empty, we'll post the request straight
// away)
void Session::add_request(std::shared_ptr<SessionRequest> req) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (!curr_req_) {
    begin_request(req);
    return;
  }

  pending_.push(req);
}

void Session::begin_request(std::shared_ptr<SessionRequest> req) {
  curr_req_ = req;

  req->set_session_id(session_id_);
  req->set_user_id(user_id_);
  req->begin(get_base_url());
}

// this is called every frame or so to update our state
void Session::update() {
  std::unique_lock<std::mutex> lock(mutex_);

  if (!curr_req_)
    return;

  SessionRequest::UpdateResult res = curr_req_->update();
  if (res == SessionRequest::kStillGoing)
    return;

  session_id_ = curr_req_->get_session_id();
  user_id_ = curr_req_->get_user_id();

  if (res == SessionRequest::kInError) {
    set_state(SessionState::kInError);
  }

  curr_req_.reset();
  if (pending_.size() > 0) {
    std::shared_ptr<SessionRequest> req = pending_.front();
    pending_.pop();

    begin_request(req);
  }
}

void Session::set_state(SessionState state) {
  state_ = state;

  // fire the signal so any listeners know
  sig_state_changed.Emit(state_);
}

std::string Session::get_base_url() const {
  std::string base_url = fw::Settings::get<std::string>("server-url");
  if (base_url[base_url.length() - 1] != '/') {
    base_url += "/";
  }
  return base_url;
}

}
