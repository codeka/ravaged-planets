#pragma once

namespace fw {
class Http;
class XmlElement;
}

namespace game {

// represents the properties of a "remote game" that we can join
class RemoteGame {
public:
  uint64_t id;
  std::string display_name;
  std::string owner_username;
  std::string owner_address;

  RemoteGame() : id(-1) {
  }
  RemoteGame(RemoteGame const &copy) :
      id(copy.id), display_name(copy.display_name), owner_username(copy.owner_username),
      owner_address(copy.owner_address) {
  }
};

// this is the base "session request" class and represents a request we'll make
// to the server to update our session state.
class SessionRequest {
public:
  typedef std::function<void(SessionRequest &req)> complete_handler_fn;

  // what we can return from the update() method
  enum UpdateResult {
    kFinished, kStillGoing, kInError
  };

private:
  complete_handler_fn _on_complete_handler;

  bool parse_response();

protected:
  std::shared_ptr<fw::Http> post_;
  std::string error_msg_;
  uint64_t session_id_;
  uint32_t user_id_;

  // converts this request into the XML that we'll post to the URL
  virtual std::string get_request_xml() {
    return "";
  }

  // gets the relative URL we post this request to (e.g. "Users/Login.ashx")
  virtual std::string get_url() {
    return "";
  }

  // gets a description of this request, used for logging
  virtual std::string get_description() {
    return "";
  }

  // parses the response from the server
  virtual bool parse_response(fw::XmlElement &xml);

public:
  SessionRequest();
  virtual ~SessionRequest();

  // posts the request, using the specific base URL
  virtual void begin(std::string base_url);

  // sets the function that we'll call when this request is complete
  void set_complete_handler(complete_handler_fn handler);

  // this is called each frame to update our state, we'll return update_result::finished
  // when the request is finished
  virtual UpdateResult update();

  // gets the error message (if there is one)
  std::string const &get_error_msg() const {
    return error_msg_;
  }

  // gets or sets the session_id, this can change (for example when you log in) and
  // the session will update itself when the request is finished.
  void set_session_id(uint64_t sid) {
    session_id_ = sid;
  }
  uint64_t get_session_id() const {
    return session_id_;
  }

  void set_user_id(uint32_t uid) {
    user_id_ = uid;
  }
  uint32_t get_user_id() const {
    return user_id_;
  }
};

// this is a request to log in to the server. You can't do anything until the login request succeeds.
class LoginSessionRequest: public SessionRequest {
private:
  std::string _username;
  std::string _password;
  int _listen_port;

protected:
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  LoginSessionRequest(std::string const &username, std::string const &password);
  virtual ~LoginSessionRequest();

  virtual void begin(std::string base_url);
};

// this is a request to log in to the server. You can't do anything until the login
// request succeeds.
class LogoutSessionRequest: public SessionRequest {
protected:
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  LogoutSessionRequest();
  virtual ~LogoutSessionRequest();

  virtual void begin(std::string base_url);
};

// this is a request to create a new game for us that people will be able to join and so on.
class CreateGameSessionRequest: public SessionRequest {
protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  CreateGameSessionRequest();
  virtual ~CreateGameSessionRequest();
};

// this is a request to refresh the list of lobbies that are available to connect to.
class ListGamesSessionRequest: public SessionRequest {
public:
  typedef std::function<void(std::vector<RemoteGame> const &games)> callback_fn;

protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

  // this is the callback we'll call once we've fetched the game list
  callback_fn callback_;

public:
  ListGamesSessionRequest(callback_fn callback);
  virtual ~ListGamesSessionRequest();
};

// this is a request to join a game, we'll get back the information required to actually
// connect to the server.
class JoinGameSessionRequest: public SessionRequest {
private:
  uint64_t game_id_;

protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  JoinGameSessionRequest(uint64_t game_id);
  ~JoinGameSessionRequest();

  virtual void begin(std::string base_url);
};

// this is used to confirm that a player is valid for a given game and that the server actually has them registered as
// a player. it returns the player# and few other details of that player as well...
class ConfirmPlayerSessionRequest: public SessionRequest {
private:
  uint64_t game_id_;
  uint32_t other_user_id_;
  std::string other_address_;
  std::string other_user_name_;
  uint8_t player_no_;
  // todo: the date/time of their login is given as well, we gotta parse it...
  bool confirmed_;

protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  ConfirmPlayerSessionRequest(uint64_t game_id, uint32_t user_id);
  ~ConfirmPlayerSessionRequest();

  bool is_confirmed() const {
    return confirmed_;
  }

  // gets the address of the other person, as detected by the login server (it should
  // be the same as the one theire connecting to us on)
  std::string get_address() const {
    return other_address_;
  }
  std::string get_user_name() const {
    return other_user_name_;
  }
  uint8_t get_player_no() const {
    return player_no_;
  }
};

}
