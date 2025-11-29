#pragma once

#include <filesystem>

#include <framework/model.h>
#include <framework/status.h>

namespace fw {

// This class is used to read models in from .mesh files.
class ModelReader {
public:
  fw::StatusOr<std::shared_ptr<Model>> read(std::filesystem::path const &filename);
};

}
