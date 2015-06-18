
#include <framework/model.h>
#include <framework/model_reader.h>
#include <framework/model_node.h>
#include <framework/texture.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/exception.h>

#include <limits>

namespace fs = boost::filesystem;

namespace fw {

std::shared_ptr<model> model_reader::read(fs::path const &filename) {
  std::shared_ptr<model> model(new fw::model());

  return model;
}

}
