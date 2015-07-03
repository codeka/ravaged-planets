#include <fstream>

#include <framework/model.h>
#include <framework/model_reader.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/exception.h>

#include <framework/model_file.pb.h>

namespace fs = boost::filesystem;

namespace fw {

void add_node(std::shared_ptr<model_node> node, Node const &pb_node);

std::shared_ptr<model> model_reader::read(fs::path const &filename) {
  std::shared_ptr<model> model(new fw::model());
  Model pb_model;

  std::fstream ins;
  ins.open(filename.c_str(), std::ios::in);
  if (ins.fail()) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::filename_error_info(filename));
  }
  pb_model.ParseFromIstream(&ins);
  ins.close();

  for (int i = 0; i < pb_model.meshes_size(); i++) {
    Mesh const &pb_mesh = pb_model.meshes(i);
    std::shared_ptr<model_mesh_noanim> mesh_noanim = std::shared_ptr<model_mesh_noanim>(new model_mesh_noanim(
        pb_mesh.vertices().size() / sizeof(vertex::xyz_n_uv),
        pb_mesh.indices().size() / sizeof(uint16_t)));
    vertex::xyz_n_uv const *vertex_begin = reinterpret_cast<vertex::xyz_n_uv const *>(pb_mesh.vertices().data());
    vertex::xyz_n_uv const *vertex_end =
        reinterpret_cast<vertex::xyz_n_uv const *>(pb_mesh.vertices().data() + pb_mesh.vertices().size());
    mesh_noanim->vertices.assign(vertex_begin, vertex_end);

    uint16_t const *indices_begin = reinterpret_cast<uint16_t const *>(pb_mesh.indices().data());
    uint16_t const *indices_end =
        reinterpret_cast<uint16_t const *>(pb_mesh.indices().data() + pb_mesh.indices().size());
    mesh_noanim->indices.assign(indices_begin, indices_end);
    model->meshes.push_back(mesh_noanim);
  }

  std::shared_ptr<model_node> root_node = std::shared_ptr<model_node>(new model_node());
  add_node(root_node, pb_model.root_node());
  model->root_node = root_node;

  return model;
}

void add_node(std::shared_ptr<model_node> node, Node const &pb_node) {
  node->colour = fw::colour::from_argb(pb_node.colour());
  node->mesh_index = pb_node.mesh_index();
  if (pb_node.transformation_size() == 16) {
    for (int i = 0; i < 16; i++) {
      node->transform.data()[i] = pb_node.transformation(i);
    }
  } else {
    node->transform = fw::identity();
  }
  node->wireframe = false;
  node->node_name = pb_node.name();

  for (int i = 0; i < pb_node.children_size(); i++) {
    Node const &pb_child = pb_node.children(i);
    std::shared_ptr<model_node> child = std::shared_ptr<model_node>(new model_node());
    add_node(child, pb_child);
    node->add_child(child);
  }
}

}
