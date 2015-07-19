
#include <framework/model_manager.h>
#include <framework/model.h>
#include <framework/model_reader.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/paths.h>

namespace fw {

std::shared_ptr<model> model_manager::get_model(std::string const &name) {
  FW_ENSURE_RENDER_THREAD();

  auto it = _models.find(name);
  if (it == _models.end()) {
    model_reader reader;
    std::shared_ptr<model> model = reader.read(fw::resolve("meshes/" + name + ".mesh"));
    model->texture = std::shared_ptr<texture>(new texture());
    model->texture->create(fw::resolve("meshes/" + name + ".png"));
    model->root_node->initialize(*model);
    _models[name] = model;
    return model;
  } else {
    return it->second;
  }
}

}
