#pragma once

#include <vector>
#include <cml/cml.h>

namespace fw {

typedef cml::matrix44f_r matrix;
typedef cml::quaternionf_p quaternion;
typedef cml::vector3f vector;
typedef cml::vector2f point;

inline matrix identity() {
  matrix m;
  cml::identity_transform(m);
  return m;
}

inline matrix rotate_axis_angle(vector const &axis, float angle) {
  matrix m;
  cml::matrix_rotation_axis_angle(m, axis, angle);
  return m;
}

inline matrix rotate(vector const &from, vector const &to) {
  matrix m;
  cml::matrix_rotation_vec_to_vec(m, from, to, true);
  return m;
}

inline matrix translation(vector const &vec) {
  matrix m;
  cml::matrix_translation(m, vec);
  return m;
}

inline matrix translation(float x, float y, float z) {
  matrix m;
  cml::matrix_translation(m, x, y, z);
  return m;
}

inline matrix scale(float s) {
  matrix m;
  cml::matrix_scale(m, s, s, s);
  return m;
}

inline matrix scale(vector const &v) {
  matrix m;
  cml::matrix_scale(m, v);
  return m;
}

}
