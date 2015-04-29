#pragma once

#include <functional>

namespace fw {
namespace vertex {

struct xyz {
  inline xyz() :
      x(0), y(0), z(0) {
  }

  inline xyz(float x, float y, float z) :
      x(x), y(y), z(z) {
  }

  inline xyz(xyz const &copy) :
      x(copy.x), y(copy.y), z(copy.z) {
  }

  float x, y, z;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_c {
  inline xyz_c() :
      x(0), y(0), z(0), colour(0) {
  }

  inline xyz_c(float x, float y, float z, uint32_t colour) :
      x(x), y(y), z(z), colour(colour) {
  }

  inline xyz_c(xyz_c const &copy) :
      x(copy.x), y(copy.y), z(copy.z), colour(copy.colour) {
  }

  float x, y, z;
  uint32_t colour;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_uv {
  inline xyz_uv() :
      x(0), y(0), z(0), u(0), v(0) {
  }

  inline xyz_uv(float x, float y, float z, float u, float v) :
      x(x), y(y), z(z), u(u), v(v) {
  }

  inline xyz_uv(xyz_uv const &copy) :
      x(copy.x), y(copy.y), z(copy.z), u(copy.u), v(copy.v) {
  }

  float x, y, z;
  float u, v;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_c_uv {
  inline xyz_c_uv() :
      x(0), y(0), z(0), colour(0), u(0), v(0) {
  }

  inline xyz_c_uv(float x, float y, float z, uint32_t colour, float u, float v) :
      x(x), y(y), z(z), colour(colour), u(u), v(v) {
  }

  inline xyz_c_uv(xyz_c_uv const &copy) :
      x(copy.x), y(copy.y), z(copy.z), colour(copy.colour), u(copy.u), v(copy.v) {
  }

  float x, y, z;
  uint32_t colour;
  float u, v;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_n_uv {
  inline xyz_n_uv() :
      x(0), y(0), z(0), nx(0), ny(0), nz(0), u(0), v(0) {
  }

  inline xyz_n_uv(float x, float y, float z, float nx, float ny, float nz,
      float u, float v) :
      x(x), y(y), z(z), nx(nx), ny(ny), nz(nz), u(u), v(v) {
  }

  inline xyz_n_uv(xyz_n_uv const &copy) :
      x(copy.x), y(copy.y), z(copy.z), nx(copy.nx), ny(copy.ny), nz(copy.nz), u(
          copy.u), v(copy.v) {
  }

  float x, y, z;
  float nx, ny, nz;
  float u, v;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_n {
  inline xyz_n() :
      x(0), y(0), z(0), nx(0), ny(0), nz(0) {
  }

  inline xyz_n(float x, float y, float z, float nx, float ny, float nz) :
      x(x), y(y), z(z), nx(nx), ny(ny), nz(nz) {
  }

  inline xyz_n(xyz_n const &copy) :
      x(copy.x), y(copy.y), z(copy.z), nx(copy.nx), ny(copy.ny), nz(copy.nz) {
  }

  float x, y, z;
  float nx, ny, nz;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

}
}
