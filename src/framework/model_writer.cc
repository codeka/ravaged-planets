#include <memory>
#include <fstream>

#include <framework/model.h>
#include <framework/model_writer.h>
#include <framework/model_node.h>
#include <framework/exception.h>

#include <framework/model_file.pb.h>

namespace fw {

void add_node(Node *pb_node, std::shared_ptr<ModelNode> Node);

void ModelWriter::write(std::string const &filename, std::shared_ptr<Model> mdl) {
  write(filename, *mdl.get());
}

void ModelWriter::write(std::string const &filename, Model &mdl) {
  ::Model pb_model;
  pb_model.set_name("TODO");
  for(auto& mesh : mdl.meshes_) {
    std::shared_ptr<ModelMeshNoanim> mesh_noanim = std::dynamic_pointer_cast<ModelMeshNoanim>(mesh);
    Mesh *pb_mesh = pb_model.add_meshes();
    pb_mesh->set_vertices(mesh_noanim->vertices.data(), mesh_noanim->vertices.size() * sizeof(vertex::xyz_n_uv));
    pb_mesh->set_indices(mesh_noanim->indices.data(), mesh_noanim->indices.size() * sizeof(uint16_t));
  }
  add_node(pb_model.mutable_root_node(), mdl.root_node_);

  std::fstream outs;
  outs.open(filename.c_str(), std::ios::out);
  if (outs.fail()) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::filename_error_info(filename));
  }
  pb_model.SerializeToOstream(&outs);
  outs.close();
}

void add_node(Node *pb_node, std::shared_ptr<ModelNode> node) {
  pb_node->set_mesh_index(node->mesh_index);
  pb_node->set_name(node->node_name);
  for (int i = 0; i < 16; i++) {
    pb_node->mutable_transformation()->Add(node->transform.data()[i]);
  }

  for (int i = 0; i < node->get_num_children(); i++) {
    std::shared_ptr<ModelNode> child_node = std::dynamic_pointer_cast<ModelNode>(node->get_child(i));
    Node *pb_child_node = pb_node->add_children();
    add_node(pb_child_node, child_node);
  }
}

}
