#include <framework/model.h>
#include <framework/model_node.h>
#include <framework/scenegraph.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/shader.h>

namespace fw {

model_mesh::model_mesh(int /*num_vertices*/, int /*num_indices*/) {
}

model_mesh::~model_mesh() {
}

//-------------------------------------------------------------------------

model_mesh_noanim::model_mesh_noanim(int num_vertices, int num_indices) :
    model_mesh(num_vertices, num_indices), vertices(num_vertices), indices(num_indices) {
}

model_mesh_noanim::~model_mesh_noanim() {
}

void model_mesh_noanim::setup_buffers() {
  if (_vb)
    return;

  _vb = vertex_buffer::create<vertex::xyz_n_uv>();
  _vb->set_data(vertices.size(), &vertices[0]);

  _ib = std::shared_ptr<index_buffer>(new index_buffer());
  _ib->set_data(indices.size(), &indices[0]);

  _shader = shader::create("entity.shader");
}

//-------------------------------------------------------------------------

model::model() {
}

model::~model() {
}

void model::render(sg::scenegraph &sg, fw::matrix const &transform /*= fw::matrix::identity() */) {
  root_node->set_world_matrix(transform);
  sg.add_node(root_node->clone());
}

void model::set_wireframe(bool value) {
  root_node->wireframe = value;
}

bool model::get_wireframe() const {
  return root_node->wireframe;
}

void model::set_colour(fw::colour col) {
  root_node->colour = col;
}

fw::colour model::get_colour() const {
  return root_node->colour;
}

}
