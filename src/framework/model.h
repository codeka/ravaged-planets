#pragma once

#include <framework/color.h>
#include <framework/graphics.h>
#include <framework/texture.h>
#include <framework/shader.h>
#include <framework/scenegraph.h>
#include <framework/vector.h>

namespace fw {
class ModelNode;

// A ModelMesh represents all the data needed for a single call to glDraw* - vertices, indices, etc.
class ModelMesh {
protected:
  std::shared_ptr<VertexBuffer> vb_;
  std::shared_ptr<IndexBuffer> ib_;
  std::shared_ptr<shader> shader_;

  virtual void setup_buffers() = 0;

public:
  ModelMesh(int num_vertices, int num_indices);
  virtual ~ModelMesh();

  std::shared_ptr<VertexBuffer> get_vertex_buffer() {
    setup_buffers();
    return vb_;
  }
  std::shared_ptr<IndexBuffer> get_index_buffer() {
    setup_buffers();
    return ib_;
  }
  std::shared_ptr<shader> get_shader() {
    setup_buffers();
    return shader_;
  }
};

// A specialization of ModelMesh that doesn't support animation.
class ModelMeshNoanim: public ModelMesh {
protected:
  virtual void setup_buffers();

public:
  ModelMeshNoanim(int num_vertices, int num_indices);
  virtual ~ModelMeshNoanim();

  std::vector<fw::vertex::xyz_n_uv> vertices;
  std::vector<uint16_t> indices;
};

// A Model consists of a hierarchy of nodes, each of which contains one or more meshes, each with their own (though
// usually the same) texture. It also contains an (optional) bone hierarchy and vertex weights, as well as a list of
// named animations.
class Model {
private:
  bool wireframe_;
  fw::Color color_;

public:
  Model();
  ~Model();

  std::vector<std::shared_ptr<fw::ModelMesh>> meshes;
  std::shared_ptr<fw::ModelNode> root_node;
  std::shared_ptr<fw::Texture> texture;

  /** Sets a ParticleRotation which indicates whether we want to render in wireframe mode or not. */
  inline void set_wireframe(bool ParticleRotation) {
    wireframe_ = ParticleRotation;
  }
  inline bool get_wireframe() const {
    return wireframe_;
  }

  /**
   * Sets the color we apply to transparent parts of the Model's texture - this is usually used to differentiate
   * different players.
   */
  inline void set_color(fw::Color col) {
    color_ = col;
  }
  inline fw::Color get_color() const {
    return color_;
  }

  /** Renders the mesh to the given Scenegraph. */
  void render(sg::Scenegraph &sg, fw::Matrix const &transform = fw::identity());
};

}
