#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/logging.h>
#include <framework/http.h>
#include <framework/xml.h>

#include <game/session/session.h>
#include <game/session/session_request.h>
#include <game/simulation/simulation_thread.h>

namespace game {

session_request::session_request() : _user_id(0), _session_id(0) {
}

session_request::~session_request() {
}

void session_request::set_complete_handler(complete_handler_fn handler) {
  _on_complete_handler = handler;
}

void session_request::begin(std::string base_url) {
  fw::xml_element xml(get_request_xml());
  _post = fw::http::perform(fw::http::POST, base_url + get_url(), xml);
  fw::debug << get_description() << std::endl;
}

session_request::update_result session_request::update() {
  if (_post->is_finished()) {
    if (!parse_response()) {
      return session_request::in_error;
    }

    // now that we're finished, call the on_complete_handler if we have one
    if (!_on_complete_handler) {
      _on_complete_handler(*this);
    }

    return session_request::finished;
  }

  return session_request::still_going;
}

bool session_request::parse_response() {
  if (_post->is_error()) {
    fw::debug << boost::format("error communicating with server: %1%") % _post->get_error_msg() << std::endl;
    _error_msg = _post->get_error_msg();
    return false;
  }

  fw::xml_element xml = _post->get_xml_response();
  if (xml.get_value() == "error") {
    fw::debug << boost::format("error returned from server: %1%") % xml.get_attribute("msg") << std::endl;
    _error_msg = xml.get_attribute("msg");
    return false;
  }

  fw::debug << xml.to_string() << std::endl;

  return parse_response(xml);
}

bool session_request::parse_response(fw::xml_element &) {
  return true;
}

//-------------------------------------------------------------------------

login_session_request::login_session_request(std::string const &username, std::string const &password) :
    _username(username), _password(password) {
  _listen_port = simulation_thread::get_instance()->get_listen_port();
}

login_session_request::~login_session_request() {
}

void login_session_request::begin(std::string base_url) {
  std::string url = (boost::format("Api/Session/New?name=%1%&password=%2%&listenPort=%3%") % _username % _password
      % _listen_port).str();

  fw::xml_element xml(get_request_xml());
  _post = fw::http::perform(fw::http::PUT, base_url + url);

  fw::debug << get_description() << std::endl;
  session::get_instance()->set_state(session::logging_in);
}

std::string login_session_request::get_description() {
  return (boost::format("logging in (username: %1%)") % _username).str();
}

bool login_session_request::parse_response(fw::xml_element &xml) {
  if (xml.get_value() != "success") {
    fw::debug << boost::format("ERR: unexpected response from login request: %1%") % xml.get_value() << std::endl;
    return false;
  }

  _session_id = boost::lexical_cast<uint64_t>(xml.get_attribute("sessionId"));
  _user_id = boost::lexical_cast<uint32_t>(xml.get_attribute("userId"));
  session::get_instance()->set_state(session::logged_in);

  fw::debug << boost::format("login successful, session-id: %1%") % _session_id << std::endl;
  return true;
}

//------------------------------------------------------------------------

logout_session_request::logout_session_request() {
}

logout_session_request::~logout_session_request() {
}

void logout_session_request::begin(std::string base_url) {
  std::string url = (boost::format("Api/Session/%1%") % _session_id).str();

  fw::xml_element xml(get_request_xml());
  _post = fw::http::perform(fw::http::DELETE, base_url + url);

  fw::debug << get_description() << std::endl;

  session::get_instance()->set_state(session::logging_out);
}

std::string logout_session_request::get_description() {
  return "logging out";
}

bool logout_session_request::parse_response(fw::xml_element &xml) {
  if (xml.get_value() != "success") {
    fw::debug << boost::format("ERR: unexpected response from logout request: %1%") % xml.get_value() << std::endl;
    return false;
  }

  session::get_instance()->set_state(session::disconnected);

  fw::debug << "logout successful" << std::endl;
  return true;
}

//-------------------------------------------------------------------------

create_game_session_request::create_game_session_request() {
}

create_game_session_request::~create_game_session_request() {
}

std::string create_game_session_request::get_request_xml() {
  return (boost::format("<game sessionId=\"%1%\" />") % _session_id).str();
}

std::string create_game_session_request::get_url() {
  return "game/create-game.php";
}

std::string create_game_session_request::get_description() {
  return "registering new game with server";
}

bool create_game_session_request::parse_response(fw::xml_element &xml) {
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
  simulation_thread::get_instance()->new_game(game_id);
  return true;
}

//----------------------------------------------------------------------------

list_games_session_request::list_games_session_request(callback_fn callback) :
    _callback(callback) {
}

list_games_session_request::~list_games_session_request() {
}

std::string list_games_session_request::get_request_xml() {
  return (boost::format("<list-games sessionId=\"%1%\" />") % _session_id).str();
}

std::string list_games_session_request::get_url() {
  return "game/list-games.php";
}

std::string list_games_session_request::get_description() {
  return "listing games";
}

bool list_games_session_request::parse_response(fw::xml_element &xml) {
  // parse out the list of games from the response
  std::vector<remote_game> games;
  for (fw::xml_element game = xml.get_first_child(); game.is_valid(); game = game.get_next_sibling()) {
    remote_game g;
    g.id = boost::lexical_cast<uint64_t>(game.get_attribute("id"));
    g.display_name = game.get_attribute("displayName");
    g.owner_username = game.get_attribute("ownerUser");
    g.owner_address = game.get_attribute("ownerAddr");
    games.push_back(g);
  }

  // call the callback with the list of games we just parsed!
  _callback(games);

  return true;
}

//-------------------------------------------------------------------------

join_game_session_request::join_game_session_request(uint64_t game_id) :
    _game_id(game_id) {
}

join_game_session_request::~join_game_session_request() {
}

std::string join_game_session_request::get_request_xml() {
  return (boost::format("<join sessionId=\"%1%\" gameId=\"%2%\" />") % _session_id % _game_id).str();
}

std::string join_game_session_request::get_url() {
  return "game/join-game.php";
}

std::string join_game_session_request::get_description() {
  return "joining game";
}

void join_game_session_request::begin(std::string base_url) {
  session_request::begin(base_url);
  session::get_instance()->set_state(session::joining_lobby);
}

bool join_game_session_request::parse_response(fw::xml_element &xml) {
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
  simulation_thread::get_instance()->connect(_game_id, server_address, player_no);

  // set the state to logged_in as well....
  session::get_instance()->set_state(session::logged_in);
  return true;
}

//-------------------------------------------------------------------------

confirm_player_session_request::confirm_player_session_request(uint64_t game_id, uint32_t user_id) :
    _game_id(game_id), _other_user_id(user_id), _player_no(0), _confirmed(false) {
}

confirm_player_session_request::~confirm_player_session_request() {
}

std::string confirm_player_session_request::get_request_xml() {
  return (boost::format("<confirm sessionId=\"%1%\" gameId=\"%2%\" otherUserId=\"%3%\" />") % _session_id % _game_id
      % _other_user_id).str();
}

std::string confirm_player_session_request::get_url() {
  return "game/confirm-player.php";
}

std::string confirm_player_session_request::get_description() {
  return (boost::format("confirming player \"%1%\"") % _other_user_id).str();
}

bool confirm_player_session_request::parse_response(fw::xml_element &xml) {
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
    _confirmed = true;

    if (xml.is_attribute_defined("addr")) {
      _other_address = xml.get_attribute("addr");
    }

    if (xml.is_attribute_defined("user")) {
      _other_user_name = xml.get_attribute("user");
    }

    if (xml.is_attribute_defined("playerNo")) {
      int player_no = boost::lexical_cast<int>(xml.get_attribute("playerNo"));
      _player_no = static_cast<uint8_t>(player_no);
    }

    fw::debug
        << boost::format("connecting player confirmed, user-id: %1%, username: %2%, player#: %3%") % _other_user_id
            % _other_user_name % static_cast<int>(_player_no) << std::endl;
  } else {
    fw::debug
        << boost::format("connecting player is not logged in to server, not allowing! user-id: %1%") % _other_user_id
        << std::endl;
    _confirmed = false;
  }

  return true;
}

}
