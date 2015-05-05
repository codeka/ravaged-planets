
#include <framework/graphics.h>
#include <framework/index_buffer.h>
#include <framework/logging.h>
#include <framework/framework.h>
#include <framework/exception.h>

namespace fw {

index_buffer::index_buffer(bool dynamic/*= false */) :
    _num_indices(0), _id(0), _dynamic(dynamic) {
  FW_CHECKED(glGenBuffers(1, &_id));
}

index_buffer::~index_buffer() {
  FW_CHECKED(glDeleteBuffers(1, &_id));
}

void index_buffer::set_data(int num_indices, uint16_t const *indices,
    int flags) {
  _num_indices = num_indices;

  if (flags <= 0)
    flags = _dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id));
  FW_CHECKED(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      num_indices * sizeof(uint16_t), reinterpret_cast<void const *>(indices), flags));
}

void index_buffer::prepare() {
  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id));
}

}
