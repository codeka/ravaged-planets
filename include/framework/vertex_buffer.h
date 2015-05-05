#pragma once

#include <functional>

#include <framework/framework.h>
#include <framework/scenegraph.h>
#include <framework/graphics.h>

namespace fw {

class index_buffer;

// A wrapper around a vertex buffer.
class vertex_buffer: private boost::noncopyable {
public:
  typedef std::function<void()> setup_fn;

private:
  GLuint _id;
  int _num_vertices;
  size_t _vertex_size;
  bool _dynamic;

  setup_fn _setup;

public:
  vertex_buffer(setup_fn setup, size_t vertex_size, bool dynamic = false);
  virtual ~vertex_buffer();

  // Helper function that makes it easier to create vertex buffers by assuming that you're passing a type
  // defined in fw::vertex::xxx (or something compatible).
  template<typename T>
  static inline std::shared_ptr<vertex_buffer> create(bool dynamic = false) {
    return std::shared_ptr<vertex_buffer>(new vertex_buffer(
        T::get_setup_function(), sizeof(T), dynamic));
  }

  void set_data(int num_vertices, void *vertices, int flags = -1);

  inline int get_num_vertices() const {
    return _num_vertices;
  }

  void bind(GLint program_location);
};

}
