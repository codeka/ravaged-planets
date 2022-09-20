#pragma once

#include <map>
#include <memory>

#include <framework/model.h>

namespace fw {

// Manages models, keeps them cached in memory and so on.
class ModelManager {
private:
  std::map<std::string, std::shared_ptr<Model>> models_;

public:
  /** Fetches the given Model, must be called on the render thread. */
  std::shared_ptr<Model> get_model(std::string const &name);
};
}
