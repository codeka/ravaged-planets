#include <memory>
#include <fstream>
#include <boost/foreach.hpp>

#include <framework/model.h>
#include <framework/model_writer.h>
#include <framework/model_node.h>
#include <framework/exception.h>

#include <framework/model_file.pb.h>

namespace fw {

void add_node(Node *pb_node, std::shared_ptr<model_node> node);

void model_writer::write(std::string const &filename, std::shared_ptr<model> mdl) {
  write(filename, *mdl.get());
}

void model_writer::write(std::string const &filename, model &mdl) {
  Model pb_model;
  pb_model.set_name("TODO");
  BOOST_FOREACH(std::shared_ptr<fw::model_mesh> mesh, mdl.meshes) {
    std::shared_ptr<model_mesh_noanim> mesh_noanim = std::dynamic_pointer_cast<model_mesh_noanim>(mesh);
    Mesh *pb_mesh = pb_model.add_meshes();
    pb_mesh->set_vertices(mesh_noanim->vertices.data(), mesh_noanim->vertices.size() * sizeof(vertex::xyz_n_uv));
    pb_mesh->set_indices(mesh_noanim->indices.data(), mesh_noanim->indices.size() * sizeof(uint16_t));
  }
  add_node(pb_model.mutable_root_node(), mdl.root_node);

  std::fstream outs;
  outs.open(filename.c_str(), std::ios::out);
  if (outs.fail()) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::filename_error_info(filename));
  }
  pb_model.SerializeToOstream(&outs);
}

void add_node(Node *pb_node, std::shared_ptr<model_node> node) {
  pb_node->set_mesh_index(node->mesh_index);
  pb_node->set_name(node->node_name);
  pb_node->set_colour(node->colour.to_argb());

  for (int i = 0; i < node->get_num_children(); i++) {
    std::shared_ptr<model_node> child_node = std::dynamic_pointer_cast<model_node>(node->get_child(i));
    Node *pb_child_node = pb_node->add_children();
    add_node(pb_child_node, child_node);
  }
}

}
