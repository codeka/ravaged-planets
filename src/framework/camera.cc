#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <framework/camera.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/logging.h>
#include <framework/input.h>
#include <framework/vector.h>
#include <framework/timer.h>

namespace fw {

camera::camera() {
  _velocity = vector(0, 0, 0);
  _max_speed = 1.0f;
  _floor_height = 0.0f;
  _updated = true;

  _position = vector(0, 0, 0);
  _up = vector(0, 1, 0);
  _right = vector(1, 0, 0);
  _forward = vector(0, 0, 1);

  _view = fw::identity();
  set_projection_matrix(cml::constantsf::pi() / 3.0f, 1.3f, 1.0f, 100.0f);
}

camera::~camera() {
}

void camera::set_ground_height(float height) {
  _floor_height = height;
}

void camera::update(float dt) {
  // cap speed to max speed
  if (_velocity.length() > _max_speed) {
    _velocity = _velocity.normalize() * _max_speed;
  }

  if (_velocity.length_squared() > 0.0f) {
    _updated = true;
  }

  if (_updated) {
    // move the camera and decelerate
    _position += _velocity;
    _velocity = vector(0, 0, 0);
    fw::vector lookat = _position + _forward;

    // calculate the new view matrix
    set_look_at(_view, _position, lookat, vector(0, 1, 0));

    // get the camera axes from the view matrix
    cml::matrix_get_transposed_basis_vectors(_view, _right, _up, _forward);

    // we've been updated now, so no need to do it again
    _updated = false;

    // fire the sig_updated signal, since we were updated
    sig_updated();
  }
}

vector camera::unproject(float x, float y) {
  cml::vector4f vec(x, y, 0.3f, 1.0f);

  matrix m = _projection;
  m.inverse();
  vec = cml::transform_vector_4D(m, vec);

  m = _view;
  m.inverse();
  vec = cml::transform_vector_4D(m, vec);

  float invw = 1.0f / vec[3];
  return vector(vec[0] * invw, vec[1] * invw, vec[2] * invw);
}

void camera::enable() {
}

void camera::disable() {
  input *inp = framework::get_instance()->get_input();
  BOOST_FOREACH(int token, _keybindings) {
    inp->unbind_key(token);
  }
  _keybindings.clear();
}

void camera::set_projection_matrix(float fov, float aspect, float near_plane, float far_plane) {
  cml::matrix_perspective_xfov_RH(_projection, fov, aspect, near_plane, far_plane, cml::z_clip_neg_one);
}
void camera::set_look_at(matrix &m, vector const &eye, vector const &look_at, vector const &up) {
  cml::matrix_look_at_RH(m, eye, look_at, up);
}

//---------------------------------------------------------------------------------------------------------

first_person_camera::first_person_camera() :
    _fly_mode(false) {
}

first_person_camera::~first_person_camera() {
}

void first_person_camera::update(float dt) {
  input *inp = framework::get_instance()->get_input();

  yaw((float) inp->mouse_dx() * dt);
  pitch((float) inp->mouse_dy() * dt);

  if (inp->key("W"))
    move_forward(-5.0f * dt);
  if (inp->key("S"))
    move_forward(5.0f * dt);
  if (inp->key("A"))
    move_right(5.0f * dt);
  if (inp->key("D"))
    move_right(-5.0f * dt);

  _position[1] = _floor_height + 1.8f;

  camera::update(dt);
}

void first_person_camera::move_forward(float units) {
  if (_fly_mode) {
    _velocity += _forward * units;
  } else {
    vector move_vector(_forward[0], 0.0f, _forward[2]);
    move_vector = move_vector.normalize() * units;
    _velocity += move_vector;
  }
}

void first_person_camera::move_right(float units) {
  _velocity += _right * units;
}

void first_person_camera::move_up(float units) {
  if (_fly_mode) {
    _velocity[1] += units;
  }
}

void first_person_camera::yaw(float radians) {
  if (radians == 0.0f)
    return;

  matrix rotation = fw::rotate_axis_angle(_up, radians);
  _right = cml::transform_vector(rotation, _right);
  _forward = cml::transform_vector(rotation, _forward);

  _updated = true;
}

void first_person_camera::pitch(float radians) {
  if (radians == 0.0f)
    return;

  matrix rotation = fw::rotate_axis_angle(_right, radians);
  _right = cml::transform_vector(rotation, _right);
  _forward = cml::transform_vector(rotation, _forward);

  _updated = true;
}

void first_person_camera::roll(float radians) {
  if (radians == 0.0f)
    return;

  matrix rotation = fw::rotate_axis_angle(_forward, radians);
  _right = cml::transform_vector(rotation, _right);
  _up = cml::transform_vector(rotation, _up);

  _updated = true;
}

//---------------------------------------------------------------------------------------------------------

lookat_camera::lookat_camera() {
  _centre = _position;
  _position += vector(-15.0f, 15.0f, 0);
}

lookat_camera::~lookat_camera() {
}

void lookat_camera::update(float dt) {
  _forward = (_centre - _position).normalize();

  camera::update(dt);
}

void lookat_camera::set_location(vector const &location) {
  vector offset = _position - _centre;
  _centre = location;
  _position = location + offset;
  _updated = true;
}

void lookat_camera::set_distance(float distance) {
  fw::vector dir(_position - _centre);
  _position = _centre + (dir.normalize() * distance);
  _updated = true;
}

float lookat_camera::get_distance() const {
  return (_position - _centre).length();
}

//---------------------------------------------------------------------------------------------------------

top_down_camera::top_down_camera() :
    _enable_mouse_move(true), _move_left(false), _move_right(false), _move_forward(false), _move_backward(false), _rotate_left(
        false), _rotate_right(false), _rotate_mouse(false), _zoom_to(0, 0, 0), _zooming(false), _last_floor_height(0.0f) {
}

top_down_camera::~top_down_camera() {
}

void top_down_camera::enable() {
  camera::enable();

  input *inp = framework::get_instance()->get_input();
  _keybindings.push_back(
      inp->bind_function("cam-forward", boost::bind(&top_down_camera::on_key_forward, this, _1, _2)));
  _keybindings.push_back(
      inp->bind_function("cam-backward", boost::bind(&top_down_camera::on_key_backward, this, _1, _2)));
  _keybindings.push_back(inp->bind_function("cam-left", boost::bind(&top_down_camera::on_key_left, this, _1, _2)));
  _keybindings.push_back(inp->bind_function("cam-right", boost::bind(&top_down_camera::on_key_right, this, _1, _2)));
  _keybindings.push_back(
      inp->bind_function("cam-rot-mouse", boost::bind(&top_down_camera::on_key_mouserotate, this, _1, _2)));
  _keybindings.push_back(
      inp->bind_function("cam-rot-left", boost::bind(&top_down_camera::on_key_rotateleft, this, _1, _2)));
  _keybindings.push_back(
      inp->bind_function("cam-rot-right", boost::bind(&top_down_camera::on_key_rotateright, this, _1, _2)));
}

void top_down_camera::disable() {
  // this call will unbind all our keys
  camera::disable();
}

int a = 0;

void top_down_camera::update(float dt) {
  input *inp = framework::get_instance()->get_input();

  if (_zooming) {
    fw::vector start = get_location();
    fw::vector dir = _zoom_to - start;
    fw::vector new_location = start + (dir * dt * 40.0f);

    if ((new_location - start).length_squared() > dir.length_squared() || dir.length_squared() < 0.5f) {
      // if the new location is further away then we were before, it
      // means we've over-shot it so just set our location to the final
      // location and we're done zooming.
      set_location(_zoom_to);
      _zooming = false;
    }

    set_location(new_location);
  }

  float rotate_around_up = 0.0f;
  float rotate_around_right = 0.0f;
  if (_rotate_mouse) {
    rotate_around_up = inp->mouse_dx() * 0.0035f;
    rotate_around_right = inp->mouse_dy() * 0.0035f;
  }

  if (_rotate_left)
    rotate_around_up -= dt;
  if (_rotate_right)
    rotate_around_up += dt;

  rotate(rotate_around_up, rotate_around_right);

  float forward = 0.0f;
  float right = 0.0f;

  if (_enable_mouse_move) {
    float mx = inp->mouse_x();
    float my = inp->mouse_y();

    if (mx <= 0)
      right -= 15.0f * dt;
    if (mx >= framework::get_instance()->get_graphics()->get_width() - 1)
      right += 15.0f * dt;
    if (my <= 0)
      forward += 15.0f * dt;
    if (my >= framework::get_instance()->get_graphics()->get_height() - 1)
      forward -= 15.0f * dt;
  }

  if (_move_forward)
    forward += 15.0f * dt;
  if (_move_backward)
    forward -= 15.0f * dt;
  if (_move_left)
    right -= 15.0f * dt;
  if (_move_right)
    right += 15.0f * dt;
  move(forward, right);

  float zoom_amount = inp->mouse_dwheel();
  if (zoom_amount > 0.1f || zoom_amount < -0.1f) {
    zoom(zoom_amount);
  }

  float df = _floor_height - _last_floor_height;
  _last_floor_height = _floor_height;
  _centre[1] += df;
  _position[1] += df;

  lookat_camera::update(dt);
}

void top_down_camera::on_key_forward(std::string, bool is_down) {
  _move_forward = is_down;
}

void top_down_camera::on_key_backward(std::string, bool is_down) {
  _move_backward = is_down;
}

void top_down_camera::on_key_left(std::string, bool is_down) {
  _move_left = is_down;
}

void top_down_camera::on_key_right(std::string, bool is_down) {
  _move_right = is_down;
}

void top_down_camera::on_key_mouserotate(std::string, bool is_down) {
  _rotate_mouse = is_down;
}

void top_down_camera::on_key_rotateleft(std::string, bool is_down) {
  _rotate_left = is_down;
}

void top_down_camera::on_key_rotateright(std::string, bool is_down) {
  _rotate_right = is_down;
}

void top_down_camera::zoom(float amount) {
  set_distance(get_distance() - amount);
}

void top_down_camera::rotate(float around_up, float around_right) {
  float max_height = 15.0f;
#ifdef _DEBUG
  // in debug mode, we let the camera more around a bit more, because it's sometimes
  // useful...
  max_height = -9999.0f;
#endif

  // we don't rotate around the right axis if we're too "low". That way,
  // we don't have to worry about viewing in the distance!
  bool allow_around_right = ((_position[1] - _centre[1]) > max_height || around_right > 0.0f);

  _position -= _centre;
  if (around_up != 0.0f) {
    matrix rotation = fw::rotate_axis_angle(vector(0, 1, 0), around_up);
    _position = cml::transform_vector(rotation, _position);
    _updated = true;
  }

  if (around_right != 0.0f && allow_around_right) {
    matrix rotation = fw::rotate_axis_angle(_right, around_right);
    _position = cml::transform_vector(rotation, _position);
    _updated = true;
  }
  _position += _centre;
}

void top_down_camera::move(float forward, float right) {
  if (forward == 0.0f && right == 0.0f)
    return;

  vector fwd = vector(_forward[0], 0, _forward[2]).normalize();

  vector movement = (_right * right) + (fwd * forward);
  _position += movement;
  _centre += movement;
  _updated = true;
}

}
