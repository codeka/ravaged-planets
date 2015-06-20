#pragma once

#include <map>
#include <memory>

namespace fw {
class model;

/** Manages models, keeps them cached in memory and so on. */
class model_manager {
private:
  std::map<std::string, std::shared_ptr<model>> _models;

public:
  std::shared_ptr<model> get_model(std::string const &name);
};
}
