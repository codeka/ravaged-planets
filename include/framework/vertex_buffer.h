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
  GLuint _name;
  int _num_vertices;
  size_t _vertex_size;
  bool _dynamic;

  setup_fn _setup;

public:
  vertex_buffer();
  virtual ~vertex_buffer();

  void create_buffer(int max_vertices, setup_fn setup, size_t vertex_size, bool dynamic = false);
  void set_data(int num_vertices, void *vertices, int flags = -1);
  void render(int num_primitives, fw::sg::primitive_type primitive_type, index_buffer *idxbuf = 0);

  // this is a helper method that makes it easier to create vertex buffers by assuming that
  // you're passing a type defined in fw::vertex::xxx (or something compatible)
  template<typename T>
  inline void create_buffer(int num_vertices, bool dynamic = false);

  inline int get_max_vertices() const {
    return 0xffffff;
  }
  inline int get_num_vertices() const {
    return _num_vertices;
  }
};

template<typename T>
void vertex_buffer::create_buffer(int num_vertices, bool dynamic /*= false*/) {
  create_buffer(num_vertices, T::get_setup_function(), sizeof(T), dynamic);
}

}
