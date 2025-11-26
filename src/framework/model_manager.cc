#include <filesystem>

#include <framework/model_manager.h>
#include <framework/model.h>
#include <framework/model_reader.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/paths.h>

namespace fs = std::filesystem;

namespace fw {

std::shared_ptr<Model> ModelManager::get_model(std::string const &name) {
  fs::path path = fw::resolve("meshes/" + name + ".mesh");
  debug << boost::format("loading mesh: %1%") % path << std::endl;

  auto it = models_.find(name);
  if (it == models_.end()) {
    ModelReader reader;
    std::shared_ptr<Model> model = reader.read(path);
    model->texture_ = std::shared_ptr<Texture>(new Texture());
    model->texture_->create(fw::resolve("meshes/" + name + ".png"));
    model->root_node_->initialize(model.get());
    models_[name] = model;
    return model;
  } else {
    return it->second;
  }
}

}
