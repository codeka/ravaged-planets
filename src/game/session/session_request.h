#pragma once

namespace fw {
class Http;
class XmlElement;
}

namespace game {

// represents the properties of a "remote game" that we can join
class remote_game {
public:
  uint64_t id;
  std::string display_name;
  std::string owner_username;
  std::string owner_address;

  remote_game() : id(-1) {
  }
  remote_game(remote_game const &copy) :
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
  enum update_result {
    finished, still_going, in_error
  };

private:
  complete_handler_fn _on_complete_handler;

  bool parse_response();

protected:
  std::shared_ptr<fw::Http> _post;
  std::string _error_msg;
  uint64_t session_id_;
  uint32_t _user_id;

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
  virtual update_result update();

  // gets the error message (if there is one)
  std::string const &get_error_msg() const {
    return _error_msg;
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
    _user_id = uid;
  }
  uint32_t get_user_id() const {
    return _user_id;
  }
};

// this is a request to log in to the server. You can't do anything until the login
// request succeeds.
class login_session_request: public SessionRequest {
private:
  std::string _username;
  std::string _password;
  int _listen_port;

protected:
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  login_session_request(std::string const &username, std::string const &password);
  virtual ~login_session_request();

  virtual void begin(std::string base_url);
};

// this is a request to log in to the server. You can't do anything until the login
// request succeeds.
class logout_session_request: public SessionRequest {
protected:
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  logout_session_request();
  virtual ~logout_session_request();

  virtual void begin(std::string base_url);
};

// this is a request to create a new game for us that people will be able to join
// and so on.
class create_game_session_request: public SessionRequest {
protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  create_game_session_request();
  virtual ~create_game_session_request();
};

// this is a request to refresh the list of lobbies that are available to connect to.
class list_games_session_request: public SessionRequest {
public:
  typedef std::function<void(std::vector<remote_game> const &games)> callback_fn;

protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

  // this is the callback we'll call once we've fetched the game list
  callback_fn _callback;

public:
  list_games_session_request(callback_fn callback);
  virtual ~list_games_session_request();
};

// this is a request to join a game, we'll get back the information required to actually
// connect to the server.
class join_game_session_request: public SessionRequest {
private:
  uint64_t _game_id;

protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  join_game_session_request(uint64_t game_id);
  ~join_game_session_request();

  virtual void begin(std::string base_url);
};

// this is used to confirm that a player is valid for a given game and that
// the server actually has them registered as a player. it returns the player#
// and few other details of that player as well...
class confirm_player_session_request: public SessionRequest {
private:
  uint64_t _game_id;
  uint32_t _other_user_id;
  std::string _other_address;
  std::string _other_user_name;
  uint8_t _player_no;
  // todo: the date/time of their login is given as well, we gotta parse it...
  bool _confirmed;

protected:
  virtual std::string get_request_xml();
  virtual std::string get_url();
  virtual std::string get_description();
  virtual bool parse_response(fw::XmlElement &xml);

public:
  confirm_player_session_request(uint64_t game_id, uint32_t user_id);
  ~confirm_player_session_request();

  bool is_confirmed() const {
    return _confirmed;
  }

  // gets the address of the other person, as detected by the login server (it should
  // be the same as the one theire connecting to us on)
  std::string get_address() const {
    return _other_address;
  }
  std::string get_user_name() const {
    return _other_user_name;
  }
  uint8_t get_player_no() const {
    return _player_no;
  }
};

}
