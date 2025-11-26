#pragma once

#include <filesystem>

#include <framework/model.h>

namespace fw {

// This class is used to read models in from .mesh files.
class ModelReader {
public:
  std::shared_ptr<Model> read(std::filesystem::path const &filename);
};

}
