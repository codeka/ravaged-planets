#pragma once

#include <vector>
#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/graphics.h>
#include <framework/vector.h>

namespace fw {

class Camera {
private:
  void set_look_at(Matrix &m, Vector const &eye, Vector const &look_at,
      Vector const &up);

protected:
  Matrix view_;
  Matrix projection_;

  Vector right_;
  Vector up_;
  Vector forward_;

  Vector position_;
  Vector lookat_;

  Vector velocity_;
  float max_speed_;

  float floor_height_;

  // this is set when the camera's matrix (etc) needs to be updated
  bool updated_;

  // these are automatically unbound when disable() is called
  std::vector<int> keybindings_;

public:
  Camera();
  virtual ~Camera();

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

  // Projects the given Screen (x,y) coordinates to a point in front of the camera in 3D space. You can then use this
  // vector to select objects, etc etc...
  Vector unproject(float x, float y);

  virtual void set_location(Vector const & location) {
    position_ = location;
    //_updated = true;
  }

  virtual Vector const & get_location() const {
    return position_;
  }

  virtual void set_direction(Vector const & direction) {
    forward_ = direction;
    //_updated = true;
  }

  virtual Vector const & get_direction() const {
    return forward_;
  }

  // "Zooms" the view to the given location. Mostly useful with a top_down_camera or lookat_camera.
  virtual void zoom_to(Vector const &location) {
    set_location(location);
  }

  Matrix const & get_view_matrix() const {
    return view_;
  }

  Matrix const & get_projection_matrix() const {
    return projection_;
  }

  // Get the "eye" position of the camera.
  Vector const & get_position() const {
    return position_;
  }

  // Gets the three directional axis.
  Vector const & get_right() const {
    return right_;
  }

  Vector const & get_forward() const {
    return forward_;
  }

  Vector const & get_up() const {
    return up_;
  }
};

// This implementation of camera is a "first-person" camera. The mouse cursor is hidden and you move the mouse to
// "look around".
class FirstPersonCamera: public Camera {
private:
  bool fly_mode_; // Fly mode means we can "fly", if false we don't move in the y-direction

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
  FirstPersonCamera();
  virtual ~FirstPersonCamera();

  virtual void update(float dt);
};

// This is a simple static camera where you tell it what to "look at" and from what direction to look from, and
// that's it - no interaction possible, etc.
class LookAtCamera: public Camera {
protected:
  // This is the "center" that we're looking at and our rotations go around.
  Vector center_;

public:
  LookAtCamera();
  virtual ~LookAtCamera();

  virtual void update(float dt);

  virtual void set_location(Vector const & location);
  virtual Vector const & get_location() const {
    return center_;
  }

  void set_distance(float distance);
  float get_distance() const;
};

// This implementation of camera is a "top-down" camera, used in RTS-style games, for example.
class TopDownCamera: public LookAtCamera {
private:
  float last_floor_height_;
  bool enable_mouse_move_;

  bool move_left_;
  bool move_right_;
  bool move_forward_;
  bool move_backward_;
  bool rotate_left_;
  bool rotate_right_;
  bool zoom_in_;
  bool zoom_out_;
  bool rotate_mouse_;

  fw::Vector zoom_to_;
  bool zooming_;

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
  TopDownCamera();
  virtual ~TopDownCamera();

  virtual void enable();
  virtual void disable();

  virtual void update(float dt);

  // "Zooms" the camera to the given location.
  virtual void zoom_to(Vector const &location) {
    zoom_to_ = location;
    zooming_ = true;
  }

  // Gets or sets a ParticleRotation which indicates whether we move the camera by moving the mouse to the edge of the Screen (as
  // well as the usual left/right/up/down keys)
  void set_mouse_move(bool ParticleRotation) {
    enable_mouse_move_ = ParticleRotation;
  }
  bool get_mouse_move() const {
    return enable_mouse_move_;
  }
};

}
