#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <framework/model.h>
#include <framework/status.h>

namespace fw {

/**
 * This class is used to writer models out to .mesh files, ready to be read back in by the
 * ModelReader class.
 */
class ModelWriter {
public:
  fw::Status write(std::filesystem::path path, Model const &model);
  fw::Status write(std::filesystem::path path, std::shared_ptr<Model> const &model);
};

}
