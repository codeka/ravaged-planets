#include <game/session/session_request.h>

#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>

#include <framework/http.h>
#include <framework/logging.h>
#include <framework/status.h>
#include <framework/xml.h>

#include <game/session/session.h>
#include <game/simulation/simulation_thread.h>

namespace game {

SessionRequest::SessionRequest() : user_id_(0), session_id_(0) {
}

SessionRequest::~SessionRequest() {
}

void SessionRequest::set_complete_handler(complete_handler_fn handler) {
  _on_complete_handler = handler;
}

void SessionRequest::begin(std::string base_url) {
  fw::XmlElement xml(get_request_xml());
  post_ = fw::Http::perform(fw::Http::POST, base_url + get_url(), xml);
  LOG(DBG) << get_description();
}

SessionRequest::UpdateResult SessionRequest::update() {
  if (post_->is_finished()) {
    const auto err = parse_response();
    if (!err.ok()) {
      LOG(ERR) << err.message() << std::endl;
      return SessionRequest::kInError;
    }

    // now that we're finished, call the on_complete_handler if we have one
    if (!_on_complete_handler) {
      _on_complete_handler(*this);
    }

    return SessionRequest::kFinished;
  }

  return SessionRequest::kStillGoing;
}

fw::Status SessionRequest::parse_response() {
  auto xml = post_->get_xml_response();
  if (!xml.ok()) {
    return xml.status();
  }
  if (xml->get_value() == "error") {
    ASSIGN_OR_RETURN(auto msg, xml->GetAttribute("msg"));
    return fw::ErrorStatus(msg);
  }

  LOG(DBG) << xml->ToString();

  return parse_response(*xml);
}

fw::Status SessionRequest::parse_response(fw::XmlElement &) {
  return fw::OkStatus();
}

//-------------------------------------------------------------------------

LoginSessionRequest::LoginSessionRequest(std::string const &username, std::string const &password) :
    _username(username), _password(password) {
  _listen_port = SimulationThread::get_instance()->get_listen_port();
}

LoginSessionRequest::~LoginSessionRequest() {
}

void LoginSessionRequest::begin(std::string base_url) {
  std::string url =
      absl::StrCat("Api/Session/New?name=", _username, "&password=", _password, "&listenPort=",
          _listen_port);

  fw::XmlElement xml(get_request_xml());
  post_ = fw::Http::perform(fw::Http::PUT, base_url + url);

  LOG(DBG) << get_description();
  Session::get_instance()->set_state(Session::kLoggingIn);
}

std::string LoginSessionRequest::get_description() {
  return absl::StrCat("logging in (username: ", _username, ")");
}

fw::Status LoginSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return fw::ErrorStatus("unexpected response from login request:") << xml.get_value();
  }

  ASSIGN_OR_RETURN(session_id_, xml.GetAttributei<uint64_t>("sessionId"));
  ASSIGN_OR_RETURN(user_id_, xml.GetAttributei<uint32_t>("userId"));
  Session::get_instance()->set_state(Session::kLoggedIn);

  LOG(INFO) << "login successful, session-id:" << session_id_;
  return fw::OkStatus();
}

//------------------------------------------------------------------------

LogoutSessionRequest::LogoutSessionRequest() {
}

LogoutSessionRequest::~LogoutSessionRequest() {
}

void LogoutSessionRequest::begin(std::string base_url) {
  std::string url = absl::StrCat("Api/Session/", session_id_);

  fw::XmlElement xml(get_request_xml());
  post_ = fw::Http::perform(fw::Http::DELETE, base_url + url);

  LOG(DBG) << get_description();

  Session::get_instance()->set_state(Session::kLoggingOut);
}

std::string LogoutSessionRequest::get_description() {
  return "logging out";
}

fw::Status LogoutSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return fw::ErrorStatus(absl::StrCat("unexpected response from logout: ", xml.get_value()));
  }

  Session::get_instance()->set_state(Session::kDisconnected);

  LOG(INFO) << "logout successful";
  return fw::OkStatus();
}

//-------------------------------------------------------------------------

CreateGameSessionRequest::CreateGameSessionRequest() {
}

CreateGameSessionRequest::~CreateGameSessionRequest() {
}

std::string CreateGameSessionRequest::get_request_xml() {
  return absl::StrCat("<game sessionId=\"", session_id_, "\" />");
}

std::string CreateGameSessionRequest::get_url() {
  return "game/create-game.php";
}

std::string CreateGameSessionRequest::get_description() {
  return "registering new game with server";
}

fw::Status CreateGameSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return fw::ErrorStatus("unexpected response from \"join game\" request: ") << xml.get_value();
  }

  ASSIGN_OR_RETURN(auto game_id, xml.GetAttributei<uint64_t>("gameId"));
  LOG(INFO) << "new game registered, identifier: " << game_id;

  // now let the simulation thread know we're starting a new game
  SimulationThread::get_instance()->new_game(game_id);
  return fw::OkStatus();
}

//----------------------------------------------------------------------------

ListGamesSessionRequest::ListGamesSessionRequest(callback_fn callback) :
    callback_(callback) {
}

ListGamesSessionRequest::~ListGamesSessionRequest() {
}

std::string ListGamesSessionRequest::get_request_xml() {
  return absl::StrCat("<list-games sessionId=\"", session_id_, "\" />");
}

std::string ListGamesSessionRequest::get_url() {
  return "game/list-games.php";
}

std::string ListGamesSessionRequest::get_description() {
  return "listing games";
}

fw::Status ListGamesSessionRequest::parse_response(fw::XmlElement &xml) {
  // parse out the list of games from the response
  std::vector<RemoteGame> games;
  for (fw::XmlElement game : xml.children()) {
    RemoteGame g;
    ASSIGN_OR_RETURN(g.id, game.GetAttributei<uint64_t>("id"));
    ASSIGN_OR_RETURN(g.display_name, game.GetAttribute("displayName"));
    ASSIGN_OR_RETURN(g.owner_username, game.GetAttribute("ownerUser"));
    ASSIGN_OR_RETURN(g.owner_address, game.GetAttribute("ownerAddr"));
    games.push_back(g);
  }

  // call the callback with the list of games we just parsed!
  callback_(games);

  return fw::OkStatus();
}

//-------------------------------------------------------------------------

JoinGameSessionRequest::JoinGameSessionRequest(uint64_t game_id) :
    game_id_(game_id) {
}

JoinGameSessionRequest::~JoinGameSessionRequest() {
}

std::string JoinGameSessionRequest::get_request_xml() {
  return absl::StrCat("<join sessionId=\"", session_id_, "\" gameId=\"", game_id_, "\" />");
}

std::string JoinGameSessionRequest::get_url() {
  return "game/join-game.php";
}

std::string JoinGameSessionRequest::get_description() {
  return "joining game";
}

void JoinGameSessionRequest::begin(std::string base_url) {
  SessionRequest::begin(base_url);
  Session::get_instance()->set_state(Session::kJoiningLobby);
}

fw::Status JoinGameSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return  fw::ErrorStatus("unexpected response from \"join game\" request: ") << xml.get_value();
  }

  ASSIGN_OR_RETURN(uint8_t player_no, xml.GetAttributei<uint8_t>("playerNo"));
  ASSIGN_OR_RETURN(std::string server_address, xml.GetAttribute("serverAddr"));
  LOG(INFO) << "joined game, address: " << server_address << " player# " << player_no;

  // now connect to that server
  RETURN_IF_ERROR(
      SimulationThread::get_instance()->connect(game_id_, server_address, player_no));

  // set the state to logged_in as well....
  Session::get_instance()->set_state(Session::kLoggedIn);
  return fw::OkStatus();
}

//-------------------------------------------------------------------------

ConfirmPlayerSessionRequest::ConfirmPlayerSessionRequest(uint64_t game_id, uint32_t user_id) :
    game_id_(game_id), other_user_id_(user_id), player_no_(0), confirmed_(false) {
}

ConfirmPlayerSessionRequest::~ConfirmPlayerSessionRequest() {
}

std::string ConfirmPlayerSessionRequest::get_request_xml() {
  return 
      absl::StrCat(
          "<confirm sessionId=\"", session_id_,
          "\" gameId=\"", game_id_,
          "\" otherUserId=\"", other_user_id_, "\" />");
}

std::string ConfirmPlayerSessionRequest::get_url() {
  return "game/confirm-player.php";
}

std::string ConfirmPlayerSessionRequest::get_description() {
  return absl::StrCat("confirming player \"", other_user_id_, "\"");
}

fw::Status ConfirmPlayerSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return
        fw::ErrorStatus("unexpected response from \"confirm player\" request:") << xml.get_value();
  }

  ASSIGN_OR_RETURN(std::string confirmed, xml.GetAttribute("confirmed"));
  if (confirmed == "true") {
    confirmed_ = true;
    ASSIGN_OR_RETURN(other_address_, xml.GetAttribute("addr"));
    ASSIGN_OR_RETURN(other_user_name_, xml.GetAttribute("user"));
    ASSIGN_OR_RETURN(player_no_, xml.GetAttributei<uint8_t>("playerNo"));

    LOG(INFO)
        << "connecting player confirmed, user-id: " << other_user_id_
        << ", username: " << other_user_name_
        << ", player#: " << player_no_;
  } else {
    LOG(INFO)
        << "connecting player is not logged in to server, not allowing! user-id: "
        << other_user_id_;
    confirmed_ = false;
  }

  return fw::OkStatus();
}

}
