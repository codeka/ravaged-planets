#pragma once

#include <cmath>

// TODO: we could probably rewrite most of this to not use linmath, but it's just easier to reuse.
#include <linmath.h>

namespace fw {

class Vector;
class Point;
class Matrix;
class Quaternion;

class Vector {
public:
  vec3 v;

  inline Vector() : v{ 0.0f, 0.0f, 0.0f } {}
  inline Vector(float x, float y, float z) : v{ x, y, z } {}

  inline float length() const {
    return vec3_len(v);
  }

  inline float& operator[](int n) {
    return v[n];
  }
  inline float operator[](int n) const {
    return v[n];
  }
  inline float x() const { return v[0]; }
  inline float y() const { return v[1]; }
  inline float z() const { return v[2]; }

  inline Vector operator*(float scalar) const {
    return Vector(v[0] * scalar, v[1] * scalar, v[2] * scalar);
  }
  inline Vector& operator*=(float scalar) {
    return (*this) = (*this) * scalar;
  }

  inline Vector operator+(const Vector& rhs) const {
    return Vector(v[0] + rhs.v[0], v[1] + rhs.v[1], v[2] + rhs.v[2]);
  }
  inline Vector& operator +=(const Vector& rhs) {
    vec3_add(v, v, rhs.v);
    return *this;
  }

  inline Vector operator-(const Vector& rhs) const {
    return Vector(v[0] - rhs.v[0], v[1] - rhs.v[1], v[2] - rhs.v[2]);
  }
  inline Vector& operator -=(const Vector& rhs) {
    vec3_sub(v, v, rhs.v);
    return (*this);
  }

  inline void normalize() {
    const float l = length();
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
  inline Vector normalized() const {
    Vector copy = *this;
    copy.normalize();
    return copy;
  }
};

inline Vector operator-(const Vector& v) {
  return v * -1;
}

// Similar to Vector, but 4 dimensional.
class Vector4 {
public:
  vec4 v;

  inline Vector4() : v{ 0.0f, 0.0f, 0.0f, 0.0f } {}
  inline Vector4(float x, float y, float z, float w) : v{ x, y, z, w } {}
  inline Vector4(const Vector& v) : v{ v[0], v[1], v[2], 1.0f } {}

  inline operator Vector() const {
    return Vector(v[0] / v[3], v[1] / v[3], v[2] / v[3]);
  }

  inline float& operator[](int n) {
    return v[n];
  }
  inline float operator[](int n) const {
    return v[n];
  }
};

// Point is a 2D vector, with almost all of the same members etc.
class Point {
public:
  vec2 v;

  inline Point() : v{ 0.0f, 0.0f } {}
  inline Point(float x, float y) : v{ x, y } {}

  inline float length() const {
    return vec2_len(v);
  }

  inline float& operator[](int n) {
    return v[n];
  }
  inline float operator[](int n) const {
    return v[n];
  }

  inline void normalize() {
    const float l = length();
    v[0] /= l;
    v[1] /= l;
  }
  inline Point normalized() const {
    Point copy = *this;
    copy.normalize();
    return copy;
  }
};

// A 4x4 matrix.
class Matrix {
public:
  mat4x4 m;

  // Note: default constructor does not initialize the values!
  inline Matrix() = default;

  inline Matrix(mat4x4 m) {
    mat4x4_dup(this->m, m);
  }

  Matrix& operator *=(const Matrix& rhs);
  Matrix operator*(const Matrix& rhs) const;
  Matrix& operator*=(const Quaternion& rhs);
  Matrix operator*(const Quaternion& rhs) const;

  inline float elem(int row, int col) const {
    return m[col][row];
  }

  // Decompose just the rotation component of this matrix into right, up and forward vectors.
  inline void decompose(Vector& forward, Vector& up, Vector& right) {
    right[0] = m[0][0];
    right[1] = m[1][0];
    right[2] = m[2][0];
    right.normalize();
    up[0] = m[0][1];
    up[1] = m[1][1];
    up[2] = m[2][1];
    up.normalize();
    forward[0] = m[0][2];
    forward[1] = m[1][2];
    forward[2] = m[2][2];
    forward.normalize();
  }

  static inline Matrix from_basis(Vector& forward, Vector& up, Vector& right) {
    // TODO: pretty sure this is wrong
    mat4x4 m = {
      { forward[0], right[0], up[0], 0.0f },
      { forward[1], right[1], up[1], 0.0f },
      { forward[2], right[2], up[2], 0.0f },
      { 0.0f, 0.0f, 0.0f, 1.0f } };
    return Matrix(m);
  }

  // Returns a new matrix that is the inverse of this one.
  inline Matrix inverse() {
    Matrix m;
    mat4x4_invert(m.m, this->m);
    return m;
  }

  inline Vector4 operator*(const Vector4& v) const {
    Vector4 res;
    mat4x4_mul_vec4(res.v, m, v.v);
    return res;
  }
  inline Vector operator*(const Vector& v) const {
    return (*this) * Vector4(v);
  }
};

// A quaternion that represents a rotation.
class Quaternion {
public:
  quat q;

  inline Quaternion() : q{ 0.0f, 0.0f, 0.0f, 1.0f } {}
  inline Quaternion(const Vector& v, float w) : q{ v[0], v[1], v[2], w } {}
  inline Quaternion(float x, float y, float z, float w) : q{ x, y, z, w } {}

  inline Quaternion& normalize() {
    quat_norm(q, q);
  }

  inline Vector operator*(const Vector& v) const {
    Vector res;
    quat_mul_vec3(res.v, q, v.v);
    return res;
  }

  inline Matrix to_matrix() const {
    Matrix m;
    mat4x4_from_quat(m.m, q);
    return m;
  }
};

inline float pi() {
  return 3.141592653f;
}

inline Matrix identity() {
  Matrix m;
  mat4x4_identity(m.m);
  return m;
}

inline float dot(const Vector& left, const Vector& right) {
  return vec3_mul_inner(left.v, right.v);
}
inline Vector cross(const Vector& left, const Vector& right) {
  Vector res;
  vec3_mul_cross(res.v, left.v, right.v);
  return res;
}

inline Quaternion rotate(const Vector& from, const Vector& to) {
  return Quaternion(cross(from.normalized(), to.normalized()), dot(from.normalized(), to.normalized()));
}

inline Quaternion rotate_axis_angle(const Vector& axis, float angle) {
  Quaternion q;
  quat_rotate(q.q, angle, axis.v);
  return q;
}

inline Matrix translation(const Vector& v) {
  Matrix m;
  mat4x4_translate(m.m, v.x(), v.y(), v.z());
  return m;
}
inline Matrix translation(float x, float y, float z) {
  Matrix m;
  mat4x4_translate(m.m, x, y, z);
  return m;
}

inline Matrix scale(float s) {
  Matrix m = identity();
  mat4x4_scale(m.m, m.m, s);
  return m;
}

inline Matrix scale(const Vector& v) {
  Matrix m = identity();
  mat4x4_scale_aniso(m.m, m.m, v.x(), v.y(), v.z());
  return m;
}

inline Matrix projection_orthographic(float left, float right, float bottom, float top, float n, float f) {
  Matrix m;
  mat4x4_ortho(m.m, left, right, bottom, top, n, f);
  return m;
}

inline Matrix projection_perspective(float fov, float aspect, float n, float f) {
  Matrix m;
  mat4x4_perspective(m.m, fov, aspect, n, f);
  return m;
}

inline Matrix look_at(const Vector& eye, const Vector& center, const Vector& up) {
  Matrix m;
  mat4x4_look_at(m.m, eye.v, center.v, up.v);
  return m;
}

// Construct a matrix that will align all vectors with the given dir. reference must be another vector that is not
// parallel to dir.
inline Matrix align(const Vector& dir, const Vector& reference) {
  Vector forward = dir.normalized();
  Vector right = cross(forward, reference);
  Vector up = cross(right, forward);
  
  Matrix m;
  mat4x4_orthonormalize(m.m, Matrix::from_basis(right, up, forward).m);
  return m;
}

inline Matrix Matrix::operator*(const Matrix& rhs) const {
  Matrix m;
  mat4x4_mul_2(m.m, rhs.m, this->m);
  return m;
}

inline Matrix& Matrix::operator*=(const Matrix& rhs) {
  mat4x4_mul(m, rhs.m, m);
  return *this;
}

inline Matrix Matrix::operator*(const Quaternion& rhs) const {
  return (*this) * rhs.to_matrix();
}

inline Matrix& Matrix::operator*=(const Quaternion& rhs) {
  return (*this) *= rhs.to_matrix();
}

}
