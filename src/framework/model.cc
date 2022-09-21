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

void ModelMeshNoanim::setup_buffers() {
  if (vb_)
    return;

  vb_ = VertexBuffer::create<vertex::xyz_n_uv>();
  vb_->set_data(vertices.size(), &vertices[0]);

  ib_ = std::shared_ptr<IndexBuffer>(new IndexBuffer());
  ib_->set_data(indices.size(), &indices[0]);

  shader_ = shader::create("entity.shader");
}

//-------------------------------------------------------------------------

Model::Model() : wireframe_(false), color_(fw::Color(1, 1, 1)) {
}

Model::~Model() {
}

void Model::render(sg::Scenegraph &sg, fw::Matrix const &transform /*= fw::matrix::identity() */) {
  root_node->set_world_matrix(transform);
  root_node->set_color(color_);
  sg.add_node(root_node->clone());
}

}
