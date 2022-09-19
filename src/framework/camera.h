#pragma once

#include <vector>
#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/graphics.h>
#include <framework/vector.h>

namespace fw {

class camera {
private:
  void set_look_at(matrix &m, vector const &eye, vector const &look_at,
      vector const &up);

protected:
  matrix _view;
  matrix _projection;

  vector _right;
  vector _up;
  vector _forward;

  vector _position;
  vector _lookat;

  vector _velocity;
  float _max_speed;

  float _floor_height;

  // this is set when the camera's matrix (etc) needs to be updated
  bool _updated;

  // these are automatically unbound when disable() is called
  std::vector<int> _keybindings;

public:
  camera();
  virtual ~camera();

  // this signal is fired when the camera has moved or rotated, etc
  boost::signals2::signal<void()> sig_updated;

  void set_projection_matrix(float fov, float aspect, float near_plane, float far_plane);
  virtual void update(float dt);
  virtual void set_ground_height(float height);
  virtual float get_ground_height() const;

  // These are called when the camera is set as the game's current camera (this is a good opportunity to bind/unbind
  // your keys).
  virtual void enable();
  virtual void disable();

  // Projects the given screen (x,y) coordinates to a point in front of the camera in 3D space. You can then use this
  // vector to select objects, etc etc...
  vector unproject(float x, float y);

  virtual void set_location(vector const & location) {
    _position = location;
    //_updated = true;
  }

  virtual vector const & get_location() const {
    return _position;
  }

  virtual void set_direction(vector const & direction) {
    _forward = direction;
    //_updated = true;
  }

  virtual vector const & get_direction() const {
    return _forward;
  }

  // "Zooms" the view to the given location. Mostly useful with a top_down_camera or lookat_camera.
  virtual void zoom_to(vector const &location) {
    set_location(location);
  }

  matrix const & get_view_matrix() const {
    return _view;
  }

  matrix const & get_projection_matrix() const {
    return _projection;
  }

  // Get the "eye" position of the camera.
  vector const & get_position() const {
    return _position;
  }

  // Gets the three directional axis.
  vector const & get_right() const {
    return _right;
  }

  vector const & get_forward() const {
    return _forward;
  }

  vector const & get_up() const {
    return _up;
  }
};

// This implementation of camera is a "first-person" camera. The mouse cursor is hidden and you move the mouse to
// "look around".
class first_person_camera: public camera {
private:
  bool _fly_mode; // Fly mode means we can "fly", if false we don't move in the y-direction

  // move in the "forward" direction (negative for backwards)
  void move_forward(float units);

  // move in the "right" direction (negative for backwards)
  void move_right(float units);

  // move in the "up" direction (negative for down)
  void move_up(float units);

  // yaw (rotate right [positive] or left [negative])
  void yaw(float radians);

  // pitch (rotate up [positive] or down [negative])
  void pitch(float radians);

  // roll (rotate clockwise [positive] or anti-clockwise [negative])
  void roll(float radians);

public:
  first_person_camera();
  virtual ~first_person_camera();

  virtual void update(float dt);
};

// This is a simple static camera where you tell it what to "look at" and from what direction to look from, and
// that's it - no interaction possible, etc.
class lookat_camera: public camera {
protected:
  // This is the "centre" that we're looking at and our rotations go around.
  vector _centre;

public:
  lookat_camera();
  virtual ~lookat_camera();

  virtual void update(float dt);

  virtual void set_location(vector const & location);
  virtual vector const & get_location() const {
    return _centre;
  }

  void set_distance(float distance);
  float get_distance() const;
};

// This implementation of camera is a "top-down" camera, used in RTS-style games, for example.
class top_down_camera: public lookat_camera {
private:
  float _last_floor_height;
  bool _enable_mouse_move;

  bool _move_left;
  bool _move_right;
  bool _move_forward;
  bool _move_backward;
  bool _rotate_left;
  bool _rotate_right;
  bool _zoom_in;
  bool _zoom_out;
  bool _rotate_mouse;

  fw::vector _zoom_to;
  bool _zooming;

  void rotate(float around_up, float around_right);
  void move(float forward, float right);
  void zoom(float amount);

  void on_key_forward(std::string keyname, bool is_down);
  void on_key_backward(std::string keyname, bool is_down);
  void on_key_left(std::string keyname, bool is_down);
  void on_key_right(std::string keyname, bool is_down);
  void on_key_mouserotate(std::string keyname, bool is_down);
  void on_key_rotateleft(std::string keyname, bool is_down);
  void on_key_rotateright(std::string keyname, bool is_down);
  void on_key_zoomin(std::string keyname, bool is_down);
  void on_key_zoomout(std::string keyname, bool is_down);

public:
  top_down_camera();
  virtual ~top_down_camera();

  virtual void enable();
  virtual void disable();

  virtual void update(float dt);

  // "Zooms" the camera to the given location.
  virtual void zoom_to(vector const &location) {
    _zoom_to = location;
    _zooming = true;
  }

  // Gets or sets a value which indicates whether we move the camera by moving the mouse to the edge of the screen (as
  // well as the usual left/right/up/down keys)
  void set_mouse_move(bool value) {
    _enable_mouse_move = value;
  }
  bool get_mouse_move() const {
    return _enable_mouse_move;
  }
};

}
