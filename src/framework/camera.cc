#include <functional>\

#include <framework/camera.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/input.h>
#include <framework/logging.h>
#include <framework/math.h>
#include <framework/timer.h>

using namespace std::placeholders;

namespace fw {

Camera::Camera() {
  velocity_ = Vector(0, 0, 0);
  max_speed_ = 1.0f;
  floor_height_ = 0.0f;
  updated_ = true;

  position_ = Vector(0, 0, 0);
  up_ = Vector(0, 1, 0);
  right_ = Vector(1, 0, 0);
  forward_ = Vector(0, 0, 1);

  view_ = fw::identity();
  projection_ = fw::projection_perspective(fw::pi() / 3.0f, 1.3f, 1.0f, 500.0f);
}

Camera::~Camera() {
}

void Camera::set_ground_height(float height) {
  floor_height_ = height;
  updated_ = true;
}

float Camera::get_ground_height() const {
  return floor_height_;
}

void Camera::update(float dt) {
  // cap speed to max speed
  if (velocity_.length() > max_speed_) {
    velocity_ = velocity_.normalized() * max_speed_;
  }

  if (velocity_.length() > 0.0f) {
    updated_ = true;
  }

  if (updated_) {
    // move the camera and decelerate
    position_ += velocity_;
    velocity_ = Vector(0, 0, 0);
    fw::Vector lookat = position_ + forward_;

    // calculate the new view matrix
    view_ = fw::look_at(position_, lookat, Vector(0, 1, 0));

    // get the camera axes from the view matrix
    view_.decompose(forward_, up_, right_);
    forward_ *= -1.0f; // TODO: why do these have to be multipled by -1?
    right_ *= -1.0f;

    // we've been updated now, so no need to do it again
    updated_ = false;

    {
      std::unique_lock lock(render_state_mutex_);
      render_state_.view = view_;
      render_state_.projection = projection_;
    }

    // fire the sig_updated signal, since we were updated
    sig_updated();
  }
}

Vector Camera::unproject(float x, float y) {
  fw::Vector4 vec(x, y, 0.3f, 1.0f);
  vec = projection_.inverse() * vec;
  vec = view_.inverse() * vec;
  return vec;
}

CameraRenderState Camera::get_render_state() {
  FW_ENSURE_RENDER_THREAD();
  std::unique_lock lock(render_state_mutex_);
  return render_state_;
}

void Camera::enable() {
}

void Camera::disable() {
  Input *inp = Framework::get_instance()->get_input();
  for(int token : keybindings_) {
    inp->unbind_key(token);
  }
  keybindings_.clear();
}

//---------------------------------------------------------------------------------------------------------

FirstPersonCamera::FirstPersonCamera() :
    fly_mode_(false) {
}

FirstPersonCamera::~FirstPersonCamera() {
}

void FirstPersonCamera::update(float dt) {
  Input *inp = Framework::get_instance()->get_input();

  yaw((float) inp->mouse_dx() * dt);
  pitch((float) inp->mouse_dy() * dt);

  if (inp->key("W"))
    move_forward(5.0f * dt);
  if (inp->key("S"))
    move_forward(-5.0f * dt);
  if (inp->key("A"))
    move_right(5.0f * dt);
  if (inp->key("D"))
    move_right(-5.0f * dt);

  position_[1] = floor_height_ + 1.8f;

  Camera::update(dt);
}

void FirstPersonCamera::move_forward(float units) {
  if (fly_mode_) {
    velocity_ += forward_ * units;
  } else {
    Vector move_vector(forward_[0], 0.0f, forward_[2]);
    move_vector = move_vector.normalized() * units;
    velocity_ += move_vector;
  }
}

void FirstPersonCamera::move_right(float units) {
  velocity_ += right_ * units;
}

void FirstPersonCamera::move_up(float units) {
  if (fly_mode_) {
    velocity_[1] += units;
  }
}

void FirstPersonCamera::yaw(float radians) {
  if (radians == 0.0f)
    return;

  Quaternion rotation = fw::rotate_axis_angle(up_, radians);
  right_ = rotation * right_;
  forward_ = rotation * forward_;

  updated_ = true;
}

void FirstPersonCamera::pitch(float radians) {
  if (radians == 0.0f)
    return;

  Quaternion rotation = fw::rotate_axis_angle(right_, radians);
  right_ = rotation * right_;
  forward_ = rotation * forward_;

  updated_ = true;
}

void FirstPersonCamera::roll(float radians) {
  if (radians == 0.0f)
    return;

  Quaternion rotation = fw::rotate_axis_angle(forward_, radians);
  right_ = rotation * right_;
  up_ = rotation * up_;

  updated_ = true;
}

//---------------------------------------------------------------------------------------------------------

LookAtCamera::LookAtCamera() {
  center_ = position_;
  position_ += Vector(-15.0f, 15.0f, 0.0f);
}

LookAtCamera::~LookAtCamera() {
}

void LookAtCamera::update(float dt) {
  forward_ = (center_ - position_).normalized();

  Camera::update(dt);
}

void LookAtCamera::set_location(Vector const &location) {
  Vector offset = position_ - center_;
  center_ = location;
  position_ = location + offset;
  updated_ = true;
}

void LookAtCamera::set_distance(float distance) {
  fw::Vector dir(position_ - center_);
  position_ = center_ + (dir.normalized() * distance);
  updated_ = true;
}

float LookAtCamera::get_distance() const {
  return (position_ - center_).length();
}

//---------------------------------------------------------------------------------------------------------

TopDownCamera::TopDownCamera() :
    enable_mouse_move_(true), move_left_(false), move_right_(false), move_forward_(false), move_backward_(false),
    rotate_left_(false), rotate_right_(false), rotate_mouse_(false), zoom_to_(0, 0, 0),
    last_floor_height_(0.0f), zooming_(false), zoom_in_(false), zoom_out_(false) {
}

TopDownCamera::~TopDownCamera() {
}

void TopDownCamera::enable() {
  Camera::enable();

  Input *inp = Framework::get_instance()->get_input();
  keybindings_.push_back(
      inp->bind_function("cam-forward", std::bind(&TopDownCamera::on_key_forward, this, _1, _2)));
  keybindings_.push_back(
      inp->bind_function("cam-backward", std::bind(&TopDownCamera::on_key_backward, this, _1, _2)));
  keybindings_.push_back(inp->bind_function("cam-left", std::bind(&TopDownCamera::on_key_left, this, _1, _2)));
  keybindings_.push_back(inp->bind_function("cam-right", std::bind(&TopDownCamera::on_key_right, this, _1, _2)));
  keybindings_.push_back(
      inp->bind_function("cam-rot-mouse", std::bind(&TopDownCamera::on_key_mouserotate, this, _1, _2)));
  keybindings_.push_back(
      inp->bind_function("cam-rot-left", std::bind(&TopDownCamera::on_key_rotateleft, this, _1, _2)));
  keybindings_.push_back(
      inp->bind_function("cam-rot-right", std::bind(&TopDownCamera::on_key_rotateright, this, _1, _2)));
  keybindings_.push_back(
      inp->bind_function("cam-zoom-in", std::bind(&TopDownCamera::on_key_zoomin, this, _1, _2)));
  keybindings_.push_back(
      inp->bind_function("cam-zoom-out", std::bind(&TopDownCamera::on_key_zoomout, this, _1, _2)));
}

void TopDownCamera::disable() {
  // this call will unbind all our keys
  Camera::disable();
}

int a = 0;

void TopDownCamera::update(float dt) {
  Input *inp = Framework::get_instance()->get_input();

  if (zooming_) {
    fw::Vector start = get_location();
    fw::Vector dir = zoom_to_ - start;
    fw::Vector new_location = start + (dir * dt * 40.0f);

    if ((new_location - start).length() > dir.length() || dir.length() < 0.5f) {
      // if the new location is further away then we were before, it
      // means we've over-shot it so just set our location to the final
      // location and we're done zooming.
      set_location(zoom_to_);
      zooming_ = false;
    }

    set_location(new_location);
  }

  float rotate_around_up = 0.0f;
  float rotate_around_right = 0.0f;
  if (rotate_mouse_) {
    rotate_around_up = inp->mouse_dx() * -0.0035f;
    rotate_around_right = inp->mouse_dy() * 0.0035f;
  }

  if (rotate_left_)
    rotate_around_up += dt;
  if (rotate_right_)
    rotate_around_up -= dt;

  rotate(rotate_around_up, rotate_around_right);

  float forward = 0.0f;
  float right = 0.0f;

  if (enable_mouse_move_) {
    float mx = inp->mouse_x();
    float my = inp->mouse_y();

    if (mx <= 0)
      right -= 15.0f * dt;
    if (mx >= Framework::get_instance()->get_graphics()->get_width() - 1)
      right += 15.0f * dt;
    if (my <= 0)
      forward -= 15.0f * dt;
    if (my >= Framework::get_instance()->get_graphics()->get_height() - 1)
      forward += 15.0f * dt;
  }

  if (move_forward_)
    forward += 15.0f * dt;
  if (move_backward_)
    forward -= 15.0f * dt;
  if (move_left_)
    right += 15.0f * dt;
  if (move_right_)
    right -= 15.0f * dt;
  move(forward, right);

  float zoom_amount = 0.0f;

  // TODO: these numbers are totally made-up, should be configuable
  zoom_amount -= inp->mouse_wheel_dx() * 1.2f;
  zoom_amount += inp->mouse_wheel_dy() * 1.2f;

  if (zoom_in_) {
    zoom_amount += 5.0f * dt;
  }
  if (zoom_out_) {
    zoom_amount -= 5.0f * dt;
  }
  if (zoom_amount > 0.001f || zoom_amount < -0.001f) {
    zoom(zoom_amount);
  }

  float df = floor_height_ - last_floor_height_;
  last_floor_height_ = floor_height_;
  center_[1] += df;
  position_[1] += df;

  LookAtCamera::update(dt);
}

void TopDownCamera::on_key_forward(std::string, bool is_down) {
  move_forward_ = is_down;
}

void TopDownCamera::on_key_backward(std::string, bool is_down) {
  move_backward_ = is_down;
}

void TopDownCamera::on_key_left(std::string, bool is_down) {
  move_left_ = is_down;
}

void TopDownCamera::on_key_right(std::string, bool is_down) {
  move_right_ = is_down;
}

void TopDownCamera::on_key_mouserotate(std::string, bool is_down) {
  rotate_mouse_ = is_down;
}

void TopDownCamera::on_key_rotateleft(std::string, bool is_down) {
  rotate_left_ = is_down;
}

void TopDownCamera::on_key_rotateright(std::string, bool is_down) {
  rotate_right_ = is_down;
}

void TopDownCamera::on_key_zoomin(std::string keyname, bool is_down) {
  zoom_in_ = is_down;
}

void TopDownCamera::on_key_zoomout(std::string keyname, bool is_down) {
  zoom_out_ = is_down;
}

void TopDownCamera::zoom(float amount) {
  set_distance(get_distance() - amount);
}

void TopDownCamera::rotate(float around_up, float around_right) {
  float max_height = 15.0f;
#ifndef DEBUG
  // we don't rotate around the right axis if we're too "low". That way, we don't have to worry about viewing in the
  // distance!
  bool allow_around_right = ((_position[1] - _centre[1]) > max_height || around_right < 0.0f);
#else
  bool allow_around_right = true;
#endif

  position_ -= center_;
  if (around_up != 0.0f) {
    Quaternion rotation = fw::rotate_axis_angle(Vector(0, 1, 0), around_up);
    position_ = rotation * position_;
    updated_ = true;
  }

  if (around_right != 0.0f && allow_around_right) {
    Quaternion rotation = fw::rotate_axis_angle(right_, around_right);
    position_ = rotation * position_;
    updated_ = true;
  }
  position_ += center_;
}

void TopDownCamera::move(float forward, float right) {
  if (forward == 0.0f && right == 0.0f)
    return;

  Vector fwd = Vector(forward_[0], 0, forward_[2]).normalized();

  Vector movement = (right_ * right) + (fwd * forward);
  position_ += movement;
  center_ += movement;
  updated_ = true;
}

}
