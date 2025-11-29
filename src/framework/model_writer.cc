#include <memory>
#include <filesystem>
#include <fstream>

#include <framework/model.h>
#include <framework/model_writer.h>
#include <framework/model_node.h>
#include <framework/exception.h>

#include <framework/model_file.pb.h>

namespace fw {
namespace {
  
void add_node(Node *pb_node, std::shared_ptr<ModelNode> node) {
  pb_node->set_mesh_index(node->mesh_index);
  pb_node->set_name(node->node_name);
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      pb_node->mutable_transformation()->Add(node->transform.elem(row, col));
    }
  }

  for (int i = 0; i < node->get_num_children(); i++) {
    auto child_node = std::dynamic_pointer_cast<ModelNode>(node->get_child(i));
    Node *pb_child_node = pb_node->add_children();
    add_node(pb_child_node, child_node);
  }
}

}  // namespace

fw::Status ModelWriter::write(std::filesystem::path path, std::shared_ptr<Model> const &model) {
  return write(path, *model.get());
}

fw::Status ModelWriter::write(std::filesystem::path path, Model const &mdl) {
  ::Model pb_model;
  pb_model.set_name(path.filename().string());
  for(auto& mesh : mdl.meshes_) {
    std::shared_ptr<ModelMeshNoanim> mesh_noanim = std::dynamic_pointer_cast<ModelMeshNoanim>(mesh);
    Mesh *pb_mesh = pb_model.add_meshes();
    pb_mesh->set_vertices(
        mesh_noanim->vertices.data(), mesh_noanim->vertices.size() * sizeof(vertex::xyz_n_uv));
    pb_mesh->set_indices(
        mesh_noanim->indices.data(), mesh_noanim->indices.size() * sizeof(uint16_t));
  }
  add_node(pb_model.mutable_root_node(), mdl.root_node_);

  std::fstream outs;
  outs.open(path, std::ios::out);
  if (outs.fail()) {
    return fw::ErrorStatus("error loading ") << path.string();
  }
  pb_model.SerializeToOstream(&outs);
  outs.close();
  return fw::OkStatus();
}

}
