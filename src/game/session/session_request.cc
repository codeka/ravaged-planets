
#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>

#include <framework/logging.h>
#include <framework/http.h>
#include <framework/xml.h>

#include <game/session/session.h>
#include <game/session/session_request.h>
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
  fw::debug << get_description() << std::endl;
}

SessionRequest::UpdateResult SessionRequest::update() {
  if (post_->is_finished()) {
    const auto err = parse_response();
    if (!err.ok()) {
      fw::debug << "ERR: " << err.message() << std::endl;
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

absl::Status SessionRequest::parse_response() {
  auto xml = post_->get_xml_response();
  if (!xml.ok()) {
    return xml.status();
  }
  if (xml->get_value() == "error") {
    return absl::InternalError(xml->get_attribute("msg"));
  }

  fw::debug << xml->to_string() << std::endl;

  return parse_response(*xml);
}

absl::Status SessionRequest::parse_response(fw::XmlElement &) {
  return absl::OkStatus();
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
      absl::StrCat("Api/Session/New?name=", _username, "&password=", _password, "&listenPort=", _listen_port);

  fw::XmlElement xml(get_request_xml());
  post_ = fw::Http::perform(fw::Http::PUT, base_url + url);

  fw::debug << get_description() << std::endl;
  Session::get_instance()->set_state(Session::kLoggingIn);
}

std::string LoginSessionRequest::get_description() {
  return absl::StrCat("logging in (username: ", _username, ")");
}

absl::Status LoginSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return absl::InvalidArgumentError(absl::StrCat("unexpected response from login request:", xml.get_value()));
  }

  session_id_ = boost::lexical_cast<uint64_t>(xml.get_attribute("sessionId"));
  user_id_ = boost::lexical_cast<uint32_t>(xml.get_attribute("userId"));
  Session::get_instance()->set_state(Session::kLoggedIn);

  fw::debug << "login successful, session-id:" << session_id_ << std::endl;
  return absl::OkStatus();
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

  fw::debug << get_description() << std::endl;

  Session::get_instance()->set_state(Session::kLoggingOut);
}

std::string LogoutSessionRequest::get_description() {
  return "logging out";
}

absl::Status LogoutSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return absl::InvalidArgumentError(absl::StrCat("unexpected response from logout request:", xml.get_value()));
  }

  Session::get_instance()->set_state(Session::kDisconnected);

  fw::debug << "logout successful" << std::endl;
  return absl::OkStatus();
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

absl::Status CreateGameSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return
        absl::InvalidArgumentError(absl::StrCat("unexpected response from \"join game\" request: ", xml.get_value()));
  }

  if (!xml.is_attribute_defined("gameId")) {
    return absl::InvalidArgumentError("create game response did not include game identifier");
  }

  uint64_t game_id = boost::lexical_cast<uint64_t>(xml.get_attribute("gameId"));
  fw::debug << "new game registered, identifier: " << game_id << std::endl;

  // now let the simulation thread know we're starting a new game
  SimulationThread::get_instance()->new_game(game_id);
  return absl::OkStatus();
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

absl::Status ListGamesSessionRequest::parse_response(fw::XmlElement &xml) {
  // parse out the list of games from the response
  std::vector<RemoteGame> games;
  for (fw::XmlElement game = xml.get_first_child(); game.is_valid(); game = game.get_next_sibling()) {
    RemoteGame g;
    g.id = boost::lexical_cast<uint64_t>(game.get_attribute("id"));
    g.display_name = game.get_attribute("displayName");
    g.owner_username = game.get_attribute("ownerUser");
    g.owner_address = game.get_attribute("ownerAddr");
    games.push_back(g);
  }

  // call the callback with the list of games we just parsed!
  callback_(games);

  return absl::OkStatus();
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

absl::Status JoinGameSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return 
        absl::InvalidArgumentError(absl::StrCat("unexpected response from \"join game\" request: ", xml.get_value()));
  }

  if (!xml.is_attribute_defined("serverAddr")) {
    return absl::InvalidArgumentError("joing game response did not include server address");
  }

  if (!xml.is_attribute_defined("playerNo")) {
    return absl::InvalidArgumentError("joining game response did not include player#");
  }

  std::string server_address = xml.get_attribute("serverAddr");

  int player_no = 0;
  if (!absl::SimpleAtoi(xml.get_attribute("playerNo"), &player_no) || player_no > 255) {
    return
        absl::InvalidArgumentError(absl::StrCat("Couldn't convert playerNo to int: ", xml.get_attribute("playerNo")));
  }
  fw::debug << "joined game, address: " << server_address << " player# " << player_no << std::endl;

  // now connect to that server
  SimulationThread::get_instance()->connect(game_id_, server_address, static_cast<uint8_t>(player_no));

  // set the state to logged_in as well....
  Session::get_instance()->set_state(Session::kLoggedIn);
  return absl::OkStatus();
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

absl::Status ConfirmPlayerSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    return
        absl::InvalidArgumentError(
            absl::StrCat("unexpected response from \"confirm player\" request:", xml.get_value()));
  }

  if (!xml.is_attribute_defined("confirmed")) {
    return absl::InvalidArgumentError("confirm player response did not include confirmation");
  }

  std::string confirmed = xml.get_attribute("confirmed");
  if (confirmed == "true") {
    confirmed_ = true;

    if (xml.is_attribute_defined("addr")) {
      other_address_ = xml.get_attribute("addr");
    }

    if (xml.is_attribute_defined("user")) {
      other_user_name_ = xml.get_attribute("user");
    }

    if (xml.is_attribute_defined("playerNo")) {
      int player_no = boost::lexical_cast<int>(xml.get_attribute("playerNo"));
      player_no_ = static_cast<uint8_t>(player_no);
    }

    fw::debug
        << "connecting player confirmed, user-id: " << other_user_id_
        << ", username: " << other_user_name_
        << ", player#: " << player_no_
        << std::endl;
  } else {
    fw::debug
        << "connecting player is not logged in to server, not allowing! user-id: "
        << other_user_id_
        << std::endl;
    confirmed_ = false;
  }

  return absl::OkStatus();
}

}
