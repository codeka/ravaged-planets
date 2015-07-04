#include <memory>
#include <boost/foreach.hpp>

#include <framework/model.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/colour.h>
#include <framework/graphics.h>
#include <framework/shader.h>

namespace fw {

model_node::model_node() :
    wireframe(false), transform(fw::identity()), mesh_index(-1) {
}

model_node::~model_node() {
}

void model_node::initialize(model &mdl) {
  if (mesh_index >= 0) {
    std::shared_ptr<model_mesh> mesh = mdl.meshes[mesh_index];
    set_vertex_buffer(mesh->get_vertex_buffer());
    set_index_buffer(mesh->get_index_buffer());
    set_shader(mesh->get_shader());
    set_primitive_type(sg::primitive_trianglelist);

    std::shared_ptr<shader_parameters> params = get_shader()->create_parameters();
    if (mdl.texture) {
      params->set_texture("entity_texture", mdl.texture);
    }
    params->set_colour("mesh_colour", fw::colour(1, 1, 1));
    set_shader_parameters(params);
  }

  BOOST_FOREACH(std::shared_ptr<node> node, _children) {
    std::dynamic_pointer_cast<model_node>(node)->initialize(mdl);
  }
}

void model_node::render(sg::scenegraph *sg) {
  if (wireframe) {
//    device = fw::framework::get_instance()->get_graphics()->get_device();
//    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  }

  fw::matrix old_world = _world;
  _world = transform * _world;

  if (mesh_index >= 0) {
    get_shader_parameters()->set_colour("mesh_colour", colour);
  }

  node::render(sg);

  _world = old_world;

  if (wireframe) {
//    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
  }
}

void model_node::populate_clone(std::shared_ptr<sg::node> clone) {
  node::populate_clone(clone);

  std::shared_ptr<model_node> mnclone(std::dynamic_pointer_cast<model_node>(clone));
  mnclone->mesh_index = mesh_index;
  mnclone->node_name = node_name;
  mnclone->wireframe = wireframe;
  mnclone->colour = colour;
  mnclone->transform = transform;
}

std::shared_ptr<sg::node> model_node::clone() {
  std::shared_ptr<sg::node> clone(new model_node());
  populate_clone(clone);
  return clone;
}

}
