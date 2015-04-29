#include <map>

#include <framework/graphics.h>
#include <framework/framework.h>
#include <framework/vertex_buffer.h>
#include <framework/index_buffer.h>
#include <framework/logging.h>
#include <framework/exception.h>

namespace fw {

static std::map<fw::sg::primitive_type, uint32_t> g_primitive_type_map;

static void ensure_primitive_type_map() {
  if (g_primitive_type_map.size() > 0)
    return;

  g_primitive_type_map[fw::sg::primitive_linestrip] = GL_LINE_STRIP;
  g_primitive_type_map[fw::sg::primitive_linelist] = GL_LINES;
  g_primitive_type_map[fw::sg::primitive_trianglelist] = GL_TRIANGLES;
  g_primitive_type_map[fw::sg::primitive_trianglestrip] = GL_TRIANGLE_STRIP;
}

vertex_buffer::vertex_buffer() :
    _num_vertices(0), _vertex_size(0), _name(0), _dynamic(false) {
}

vertex_buffer::~vertex_buffer() {
  if (_name != 0)
    glDeleteBuffers(1, &_name);
}

void vertex_buffer::create_buffer(int max_vertices, setup_fn setup, size_t vertex_size, bool dynamic) {
  _setup = setup;

  if (_name != 0) {
    FW_CHECKED(glDeleteBuffers(1, &_name));
    _name = 0;
  }
  FW_CHECKED(glGenBuffers(1, &_name));

  _num_vertices = 0;
  _vertex_size = vertex_size;
  _dynamic = dynamic;
}

void vertex_buffer::set_data(int num_vertices, void *vertices, int flags /*= -1*/) {
  if (_name == 0) {
    BOOST_THROW_EXCEPTION(fw::exception()
        << fw::message_error_info("Cannot vertex_buffer::set_data() before create_buffer() is called."));
  }

  if (flags <= 0) {
    flags = _dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
  }

  _num_vertices = num_vertices;

  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, _name));
  FW_CHECKED(glBufferData(GL_ARRAY_BUFFER, _num_vertices * _vertex_size, vertices, flags));
}

void vertex_buffer::render(int num_primitives, fw::sg::primitive_type pt, index_buffer *idxbuf /*= 0*/) {
  ensure_primitive_type_map();
  if (_name == 0) {
    BOOST_THROW_EXCEPTION(fw::exception()
        << fw::message_error_info("Tried to render uninitialized vertex_buffer."));
    return;
  }

  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, _name));

  if (_setup) {
    _setup();
  }

  FW_CHECKED(glEnableClientState(GL_VERTEX_ARRAY));

  if (idxbuf != 0) {
    idxbuf->prepare();
    FW_CHECKED(glEnableClientState(GL_INDEX_ARRAY));
    FW_CHECKED(glDrawRangeElements(
        g_primitive_type_map[pt], 0, idxbuf->get_num_indices(), num_primitives, GL_UNSIGNED_SHORT, 0));
    FW_CHECKED(glDisableClientState(GL_INDEX_ARRAY));
  } else {
    FW_CHECKED(glDrawArrays(g_primitive_type_map[pt], 0, num_primitives));
  }

  FW_CHECKED(glDisableClientState(GL_VERTEX_ARRAY));
}

}
