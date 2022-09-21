#include <memory>

#include <framework/model.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/color.h>
#include <framework/graphics.h>
#include <framework/shader.h>

namespace fw {

ModelNode::ModelNode() : transform(fw::identity()), mesh_index(-1) {
}

ModelNode::~ModelNode() {
}

void ModelNode::initialize(Model *mdl) {
  model_ = mdl;

  if (mesh_index >= 0) {
    std::shared_ptr<ModelMesh> mesh = mdl->meshes[mesh_index];
    set_vertex_buffer(mesh->get_vertex_buffer());
    set_index_buffer(mesh->get_index_buffer());
    set_shader(mesh->get_shader());
    set_primitive_type(sg::kTriangleList);

    std::shared_ptr<ShaderParameters> params = get_shader()->create_parameters();
    if (mdl->texture) {
      params->set_texture("entity_texture", mdl->texture);
    }
    params->set_color("mesh_color", fw::Color(1, 1, 1));
    set_shader_parameters(params);
  }

  for(std::shared_ptr<Node> Node : _children) {
    std::dynamic_pointer_cast<ModelNode>(Node)->initialize(mdl);
  }
}

void ModelNode::render(sg::Scenegraph *sg, fw::Matrix const &model_matrix /*= fw::identity()*/) {
  if (model_->get_wireframe()) {
//    device = fw::framework::get_instance()->get_graphics()->get_device();
//    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  }

  if (mesh_index >= 0) {
    get_shader_parameters()->set_color("mesh_color", color_);
  }

  Node::render(sg, transform * model_matrix);

  if (model_->get_wireframe()) {
//    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
  }
}

void ModelNode::populate_clone(std::shared_ptr<sg::Node> clone) {
  Node::populate_clone(clone);

  std::shared_ptr<ModelNode> mnclone(std::dynamic_pointer_cast<ModelNode>(clone));
  mnclone->model_ = model_;
  mnclone->mesh_index = mesh_index;
  mnclone->node_name = node_name;
  mnclone->transform = transform;
  mnclone->color_ = color_;
}

void ModelNode::set_color(fw::Color color) {
  color_ = color;
  for(std::shared_ptr<Node> &child_node: _children) {
    std::dynamic_pointer_cast<ModelNode>(child_node)->set_color(color);
  }
}

std::shared_ptr<sg::Node> ModelNode::clone() {
  std::shared_ptr<sg::Node> clone(new ModelNode());
  populate_clone(clone);
  return clone;
}

}
