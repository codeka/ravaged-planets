#pragma once

#include <framework/color.h>
#include <framework/graphics.h>
#include <framework/math.h>
#include <framework/shader.h>
#include <framework/scenegraph.h>
#include <framework/texture.h>

namespace fw {
class ModelManager;
class ModelNode;

// A ModelMesh represents all the data needed for a single call to glDraw* - vertices, indices, etc.
class ModelMesh {
protected:
  std::shared_ptr<VertexBuffer> vb_;
  std::shared_ptr<IndexBuffer> ib_;
  std::shared_ptr<Shader> shader_;

  virtual void SetupBuffers() = 0;

public:
  ModelMesh(int num_vertices, int num_indices);
  virtual ~ModelMesh();

  std::shared_ptr<VertexBuffer> get_vertex_buffer() {
    SetupBuffers();
    return vb_;
  }
  std::shared_ptr<IndexBuffer> get_index_buffer() {
    SetupBuffers();
    return ib_;
  }
  std::shared_ptr<Shader> get_shader() {
    SetupBuffers();
    return shader_;
  }
};

// A specialization of ModelMesh that doesn't support animation.
class ModelMeshNoanim: public ModelMesh {
protected:
  virtual void SetupBuffers();

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

  friend class ModelManager;
  friend class ModelNode;
  friend class ModelWriter;

  std::shared_ptr<fw::Texture> texture_;
  const std::vector<std::shared_ptr<fw::ModelMesh>> meshes_;
  const std::shared_ptr<fw::ModelNode> root_node_;

public:
  Model(const std::vector<std::shared_ptr<fw::ModelMesh>>& meshes, std::shared_ptr<fw::ModelNode> root_node);
  ~Model();

  // Creates a new scenegraph node you can use that will render this model.
  std::shared_ptr<fw::ModelNode> create_node(fw::Color color);
};

}
