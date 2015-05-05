#include <map>

#include <framework/graphics.h>
#include <framework/framework.h>
#include <framework/vertex_buffer.h>
#include <framework/index_buffer.h>
#include <framework/logging.h>
#include <framework/exception.h>

namespace fw {


vertex_buffer::vertex_buffer(setup_fn setup, size_t vertex_size, bool dynamic /*= false */) :
    _num_vertices(0), _vertex_size(vertex_size), _id(0), _dynamic(dynamic), _setup(setup) {
  FW_CHECKED(glGenBuffers(1, &_id));
}

vertex_buffer::~vertex_buffer() {
  FW_CHECKED(glDeleteVertexArrays(1, &_id));
}

void vertex_buffer::set_data(int num_vertices, void *vertices, int flags /*= -1*/) {
  if (flags <= 0) {
    flags = _dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
  }

  _num_vertices = num_vertices;

  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, _id));
  FW_CHECKED(glBufferData(GL_ARRAY_BUFFER, _num_vertices * _vertex_size, vertices, flags));
}

void vertex_buffer::bind(GLint program_location) {
  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, _id));
  _setup();
}

}
