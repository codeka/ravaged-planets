#include <filesystem>
#include <fstream>

#include <framework/model.h>
#include <framework/model_reader.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/exception.h>

#include <framework/model_file.pb.h>

namespace fs = std::filesystem;

namespace fw {

void add_node(std::shared_ptr<ModelNode> node, Node const &pb_node);

fw::StatusOr<std::shared_ptr<Model>> ModelReader::read(fs::path const &filename) {
  ::Model pb_model;

  std::fstream ins;
  ins.open(filename.c_str(), std::ios::in | std::ifstream::binary);
  if (ins.fail()) {
    return fw::ErrorStatus("error opening file: ") << filename.string();
  }
  pb_model.ParseFromIstream(&ins);
  ins.close();

  std::vector<std::shared_ptr<fw::ModelMesh>> meshes;
  for (int i = 0; i < pb_model.meshes_size(); i++) {
    Mesh const &pb_mesh = pb_model.meshes(i);
    std::shared_ptr<ModelMeshNoanim> mesh_noanim =
        std::make_shared<ModelMeshNoanim>(
            pb_mesh.vertices().size() / sizeof(vertex::xyz_n_uv),
            pb_mesh.indices().size() / sizeof(uint16_t));
    vertex::xyz_n_uv const *vertex_begin =
        reinterpret_cast<vertex::xyz_n_uv const *>(pb_mesh.vertices().data());
    vertex::xyz_n_uv const *vertex_end =
        reinterpret_cast<vertex::xyz_n_uv const *>(
            pb_mesh.vertices().data() + pb_mesh.vertices().size());
    mesh_noanim->vertices.assign(vertex_begin, vertex_end);

    uint16_t const *indices_begin = reinterpret_cast<uint16_t const *>(pb_mesh.indices().data());
    uint16_t const *indices_end =
        reinterpret_cast<uint16_t const *>(pb_mesh.indices().data() + pb_mesh.indices().size());
    mesh_noanim->indices.assign(indices_begin, indices_end);
    meshes.push_back(mesh_noanim);
  }

  std::shared_ptr<ModelNode> root_node = std::shared_ptr<ModelNode>(new ModelNode());
  add_node(root_node, pb_model.root_node());
  
  return std::make_shared<fw::Model>(meshes, root_node);
}

void add_node(std::shared_ptr<ModelNode> node, Node const &pb_node) {
  node->mesh_index = pb_node.mesh_index();
  if (pb_node.transformation_size() == 16) {
    float mat[4][4];
    for (int i = 0; i < 16; i++) {
      mat[i / 4][i % 4] = pb_node.transformation(i);
    }
    node->transform = fw::Matrix(mat);
  } else {
    node->transform = fw::identity();
  }
  node->node_name = pb_node.name();

  for (int i = 0; i < pb_node.children_size(); i++) {
    Node const &pb_child = pb_node.children(i);
    std::shared_ptr<ModelNode> child = std::shared_ptr<ModelNode>(new ModelNode());
    add_node(child, pb_child);
    node->add_child(child);
  }
}

}
