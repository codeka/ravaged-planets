#pragma once

#include <framework/graphics.h>
#include <framework/model.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>
#include <framework/vector.h>

namespace fw {

/**
 * This is a specialization of the Scenegraph Node used by models. It basically just contains a bit of extra info
 * that we want to keep around to make loading/saving them easier.
 */
class ModelNode: public sg::Node {
private:
  Model *model_;
  fw::Color color_;

protected:
  /* Renders the Model Node. */
  virtual void render(sg::Scenegraph *sg, fw::Matrix const &model_matrix = fw::identity());

  /** Called by clone() to populate the clone. */
  virtual void populate_clone(std::shared_ptr<sg::Node> clone);
public:
  ModelNode();
  virtual ~ModelNode();

  /** The index into the array of meshes that we got our data from. */
  int mesh_index;

  /** The name of this Node, used to reference this Node in other parts of the Model, usually (e.g. in bones). */
  std::string node_name;

  /** The transform used to move the Model into position in the world. */
  fw::Matrix transform;

  void set_color(fw::Color color);

  /** You can call this after setting mesh_index to set up the Node. */
  void initialize(Model *mdl);

  virtual std::shared_ptr<sg::Node> clone();
};

}
