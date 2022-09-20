#pragma once

#include <memory>
#include <string>

#include <framework/model.h>

namespace fw {

/** This class is used to writer models out to .mesh files, ready to be read back in by the ModelReader class. */
class ModelWriter {
public:
  void write(std::string const &filename, Model &mdl);
  void write(std::string const &filename, std::shared_ptr<Model> mdl);
};

}
