#include <framework/model.h>
#include <framework/model_node.h>
#include <framework/scenegraph.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/shader.h>

namespace fw {

ModelMesh::ModelMesh(int /*num_vertices*/, int /*num_indices*/) {
}

ModelMesh::~ModelMesh() {
}

//-------------------------------------------------------------------------

ModelMeshNoanim::ModelMeshNoanim(int num_vertices, int num_indices) :
  ModelMesh(num_vertices, num_indices), vertices(num_vertices), indices(num_indices) {
}

ModelMeshNoanim::~ModelMeshNoanim() {
}

void ModelMeshNoanim::SetupBuffers() {
  if (vb_)
    return;

  vb_ = VertexBuffer::create<vertex::xyz_n_uv>();
  vb_->set_data(vertices.size(), &vertices[0]);

  ib_ = std::shared_ptr<IndexBuffer>(new IndexBuffer());
  ib_->set_data(indices.size(), &indices[0]);

  shader_ = Shader::CreateOrEmpty("entity.shader");
}

//-------------------------------------------------------------------------

Model::Model(const std::vector<std::shared_ptr<fw::ModelMesh>>& meshes, std::shared_ptr<fw::ModelNode> root_node)
  : meshes_(meshes), root_node_(root_node) {
}

Model::~Model() {
}

std::shared_ptr<fw::ModelNode> Model::create_node(fw::Color color) {
  auto clone = std::dynamic_pointer_cast<fw::ModelNode>(root_node_->clone());
  clone->set_color(color);
  return clone;
}

}
