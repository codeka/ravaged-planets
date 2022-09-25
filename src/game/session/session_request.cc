#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

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
    if (!parse_response()) {
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

bool SessionRequest::parse_response() {
  if (post_->is_error()) {
    fw::debug << boost::format("error communicating with server: %1%") % post_->get_error_msg() << std::endl;
    error_msg_ = post_->get_error_msg();
    return false;
  }

  fw::XmlElement xml = post_->get_xml_response();
  if (xml.get_value() == "error") {
    fw::debug << boost::format("error returned from server: %1%") % xml.get_attribute("msg") << std::endl;
    error_msg_ = xml.get_attribute("msg");
    return false;
  }

  fw::debug << xml.to_string() << std::endl;

  return parse_response(xml);
}

bool SessionRequest::parse_response(fw::XmlElement &) {
  return true;
}

//-------------------------------------------------------------------------

LoginSessionRequest::LoginSessionRequest(std::string const &username, std::string const &password) :
    _username(username), _password(password) {
  _listen_port = SimulationThread::get_instance()->get_listen_port();
}

LoginSessionRequest::~LoginSessionRequest() {
}

void LoginSessionRequest::begin(std::string base_url) {
  std::string url = (boost::format("Api/Session/New?name=%1%&password=%2%&listenPort=%3%") % _username % _password
      % _listen_port).str();

  fw::XmlElement xml(get_request_xml());
  post_ = fw::Http::perform(fw::Http::PUT, base_url + url);

  fw::debug << get_description() << std::endl;
  Session::get_instance()->set_state(Session::kLoggingIn);
}

std::string LoginSessionRequest::get_description() {
  return (boost::format("logging in (username: %1%)") % _username).str();
}

bool LoginSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    fw::debug << boost::format("ERR: unexpected response from login request: %1%") % xml.get_value() << std::endl;
    return false;
  }

  session_id_ = boost::lexical_cast<uint64_t>(xml.get_attribute("sessionId"));
  user_id_ = boost::lexical_cast<uint32_t>(xml.get_attribute("userId"));
  Session::get_instance()->set_state(Session::kLoggedIn);

  fw::debug << boost::format("login successful, session-id: %1%") % session_id_ << std::endl;
  return true;
}

//------------------------------------------------------------------------

LogoutSessionRequest::LogoutSessionRequest() {
}

LogoutSessionRequest::~LogoutSessionRequest() {
}

void LogoutSessionRequest::begin(std::string base_url) {
  std::string url = (boost::format("Api/Session/%1%") % session_id_).str();

  fw::XmlElement xml(get_request_xml());
  post_ = fw::Http::perform(fw::Http::DELETE, base_url + url);

  fw::debug << get_description() << std::endl;

  Session::get_instance()->set_state(Session::kLoggingOut);
}

std::string LogoutSessionRequest::get_description() {
  return "logging out";
}

bool LogoutSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    fw::debug << boost::format("ERR: unexpected response from logout request: %1%") % xml.get_value() << std::endl;
    return false;
  }

  Session::get_instance()->set_state(Session::kDisconnected);

  fw::debug << "logout successful" << std::endl;
  return true;
}

//-------------------------------------------------------------------------

CreateGameSessionRequest::CreateGameSessionRequest() {
}

CreateGameSessionRequest::~CreateGameSessionRequest() {
}

std::string CreateGameSessionRequest::get_request_xml() {
  return (boost::format("<game sessionId=\"%1%\" />") % session_id_).str();
}

std::string CreateGameSessionRequest::get_url() {
  return "game/create-game.php";
}

std::string CreateGameSessionRequest::get_description() {
  return "registering new game with server";
}

bool CreateGameSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    fw::debug << boost::format("ERR: unexpected response from \"join game\" request: %1%") % xml.get_value()
        << std::endl;
    return false;
  }

  if (!xml.is_attribute_defined("gameId")) {
    fw::debug << "ERR: create game response did not include game identifier" << std::endl;
    return false;
  }

  uint64_t game_id = boost::lexical_cast<uint64_t>(xml.get_attribute("gameId"));
  fw::debug << boost::format("new game registered, identifier: %1%") % game_id << std::endl;

  // now let the simulation thread know we're starting a new game
  SimulationThread::get_instance()->new_game(game_id);
  return true;
}

//----------------------------------------------------------------------------

ListGamesSessionRequest::ListGamesSessionRequest(callback_fn callback) :
    callback_(callback) {
}

ListGamesSessionRequest::~ListGamesSessionRequest() {
}

std::string ListGamesSessionRequest::get_request_xml() {
  return (boost::format("<list-games sessionId=\"%1%\" />") % session_id_).str();
}

std::string ListGamesSessionRequest::get_url() {
  return "game/list-games.php";
}

std::string ListGamesSessionRequest::get_description() {
  return "listing games";
}

bool ListGamesSessionRequest::parse_response(fw::XmlElement &xml) {
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

  return true;
}

//-------------------------------------------------------------------------

JoinGameSessionRequest::JoinGameSessionRequest(uint64_t game_id) :
    game_id_(game_id) {
}

JoinGameSessionRequest::~JoinGameSessionRequest() {
}

std::string JoinGameSessionRequest::get_request_xml() {
  return (boost::format("<join sessionId=\"%1%\" gameId=\"%2%\" />") % session_id_ % game_id_).str();
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

bool JoinGameSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    fw::debug << boost::format("ERR: unexpected response from \"join game\" request: %1%") % xml.get_value()
        << std::endl;
    return false;
  }

  if (!xml.is_attribute_defined("serverAddr")) {
    fw::debug << "ERR: joing game response did not include server address" << std::endl;
    return false;
  }

  if (!xml.is_attribute_defined("playerNo")) {
    fw::debug << "ERR: joining game response did not include player#" << std::endl;
    return false;
  }

  std::string server_address = xml.get_attribute("serverAddr");
  uint8_t player_no = static_cast<uint8_t>(boost::lexical_cast<int>(xml.get_attribute("playerNo")));
  fw::debug << boost::format("joined game, address: %1% player# %2%") % server_address % static_cast<int>(player_no)
      << std::endl;

  // now connect to that server
  SimulationThread::get_instance()->connect(game_id_, server_address, player_no);

  // set the state to logged_in as well....
  Session::get_instance()->set_state(Session::kLoggedIn);
  return true;
}

//-------------------------------------------------------------------------

ConfirmPlayerSessionRequest::ConfirmPlayerSessionRequest(uint64_t game_id, uint32_t user_id) :
    game_id_(game_id), other_user_id_(user_id), player_no_(0), confirmed_(false) {
}

ConfirmPlayerSessionRequest::~ConfirmPlayerSessionRequest() {
}

std::string ConfirmPlayerSessionRequest::get_request_xml() {
  return (boost::format("<confirm sessionId=\"%1%\" gameId=\"%2%\" otherUserId=\"%3%\" />") % session_id_ % game_id_
      % other_user_id_).str();
}

std::string ConfirmPlayerSessionRequest::get_url() {
  return "game/confirm-player.php";
}

std::string ConfirmPlayerSessionRequest::get_description() {
  return (boost::format("confirming player \"%1%\"") % other_user_id_).str();
}

bool ConfirmPlayerSessionRequest::parse_response(fw::XmlElement &xml) {
  if (xml.get_value() != "success") {
    fw::debug << boost::format("ERR: unexpected response from \"confirm player\" request: %1%") % xml.get_value()
        << std::endl;
    return false;
  }

  if (!xml.is_attribute_defined("confirmed")) {
    fw::debug << "ERR: confirm player response did not include confirmation" << std::endl;
    return false;
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
        << boost::format("connecting player confirmed, user-id: %1%, username: %2%, player#: %3%") % other_user_id_
            % other_user_name_ % static_cast<int>(player_no_) << std::endl;
  } else {
    fw::debug
        << boost::format("connecting player is not logged in to server, not allowing! user-id: %1%") % other_user_id_
        << std::endl;
    confirmed_ = false;
  }

  return true;
}

}
