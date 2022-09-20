#include <boost/filesystem.hpp>

#include <framework/model_manager.h>
#include <framework/model.h>
#include <framework/model_reader.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/paths.h>

namespace fs = boost::filesystem;

namespace fw {

std::shared_ptr<Model> ModelManager::get_model(std::string const &name) {
  FW_ENSURE_RENDER_THREAD();
  fs::path path = fw::resolve("meshes/" + name + ".mesh");
  debug << boost::format("loading mesh: %1%") % path << std::endl;

  auto it = models_.find(name);
  if (it == models_.end()) {
    ModelReader reader;
    std::shared_ptr<Model> Model = reader.read(path);
    Model->texture = std::shared_ptr<Texture>(new Texture());
    Model->texture->create(fw::resolve("meshes/" + name + ".png"));
    Model->root_node->initialize(Model.get());
    models_[name] = Model;
    return Model;
  } else {
    return it->second;
  }
}

}
