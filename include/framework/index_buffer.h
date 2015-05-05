#pragma once

#include <framework/graphics.h>

namespace fw {

class graphics;

// An index buffer holds lists of indices into a vertex_buffer.
class index_buffer: private boost::noncopyable {
private:
  GLuint _id;
  int _num_indices;
  bool _dynamic;

public:
  index_buffer(bool dynamic = false);
  ~index_buffer();

  void set_data(int num_indices, uint16_t const *indices, int flags = -1);

  inline int get_num_indices() const {
    return _num_indices;
  }

  // Called just before and just after we're going to render with this index buffer.
  void begin();
  void end();
};

}
