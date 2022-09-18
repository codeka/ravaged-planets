#pragma once

namespace fw {
class model;

/** This class is used to writer models out to .mesh files, ready to be read back in by the model_reader class. */
class model_writer {
public:
  void write(std::string const &filename, model &mdl);
  void write(std::string const &filename, std::shared_ptr<model> mdl);
};

}
