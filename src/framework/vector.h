#pragma once

#include <vector>
#include <cml/cml.h>

namespace fw {

typedef cml::matrix44f_r Matrix;
typedef cml::quaternionf_p Quaternion;
typedef cml::vector3f Vector;
typedef cml::vector2f Point;

inline Matrix identity() {
  Matrix m;
  cml::identity_transform(m);
  return m;
}

inline Matrix rotate_axis_angle(Vector const &axis, float angle) {
  Matrix m;
  cml::matrix_rotation_axis_angle(m, axis, angle);
  return m;
}

inline Matrix rotate(Vector const &from, Vector const &to) {
  Matrix m;
  cml::matrix_rotation_vec_to_vec(m, from, to, true);
  return m;
}

inline Matrix translation(Vector const &vec) {
  Matrix m;
  cml::matrix_translation(m, vec);
  return m;
}

inline Matrix translation(float x, float y, float z) {
  Matrix m;
  cml::matrix_translation(m, x, y, z);
  return m;
}

inline Matrix scale(float s) {
  Matrix m;
  cml::matrix_scale(m, s, s, s);
  return m;
}

inline Matrix scale(Vector const &v) {
  Matrix m;
  cml::matrix_scale(m, v);
  return m;
}

}
