#pragma once

#include <boost/filesystem.hpp>

#include <framework/model.h>

namespace fw {

/** This class is used to read models in from .mesh files. */
class model_reader {
public:
  std::shared_ptr<model> read(boost::filesystem::path const &filename);
};

}
