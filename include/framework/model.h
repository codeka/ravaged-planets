#pragma once

#include <framework/graphics.h>
#include <framework/vector.h>
#include <framework/colour.h>

namespace fw {
class vertex_buffer;
class index_buffer;
class effect;
class model_node;
class texture;
namespace sg {
class scenegraph;
class node;
}

/** A model_mesh represents all the data needed for a single call to glDraw* - vertices, indices, etc. */
class model_mesh {
protected:
  std::shared_ptr<vertex_buffer> _vb;
  std::shared_ptr<index_buffer> _ib;
  std::shared_ptr<effect> _fx;

  virtual void setup_buffers() = 0;

public:
  model_mesh(int num_vertices, int num_indices);
  virtual ~model_mesh();

  std::shared_ptr<vertex_buffer> get_vertex_buffer() {
    setup_buffers();
    return _vb;
  }
  std::shared_ptr<index_buffer> get_index_buffer() {
    setup_buffers();
    return _ib;
  }
  std::shared_ptr<effect> get_effect() {
    setup_buffers();
    return _fx;
  }
};

/** A specialization of model_mesh that doesn't support animation. */
class model_mesh_noanim: public model_mesh {
protected:
  virtual void setup_buffers();

public:
  model_mesh_noanim(int num_vertices, int num_indices);
  virtual ~model_mesh_noanim();

  std::vector<fw::vertex::xyz_n_uv> vertices;
  std::vector<uint16_t> indices;
};

/**
 * A model consists of a hierarchy of nodes, each of which contains one or more meshes, each with their own (though
 * usually the same) texture. It also contains an (optional) bone hierarchy and vertex weights, as well as a list of
 * named animations.
 */
class model {
public:
  model();
  ~model();

  std::vector<std::shared_ptr<model_mesh>> meshes;
  std::shared_ptr<model_node> root_node;
  std::shared_ptr<texture> texture;

  /** Sets a value which indicates whether we want to render in wireframe mode or not. */
  void set_wireframe(bool value);
  bool get_wireframe() const;

  /**
   * Sets the colour we apply to transparent parts of the model's texture - this is usually used to differentiate
   * different players.
   */
  void set_colour(fw::colour col);
  fw::colour get_colour() const;

  /** Renders the mesh to the given scenegraph. */
  void render(sg::scenegraph &sg, fw::matrix const &transform = fw::identity());
};

}
