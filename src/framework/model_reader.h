#pragma once

#include <boost/filesystem.hpp>

#include <framework/model.h>

namespace fw {

// This class is used to read models in from .mesh files.
class ModelReader {
public:
  std::shared_ptr<Model> read(boost::filesystem::path const &filename);
};

}
