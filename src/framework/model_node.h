#pragma once

#include <framework/graphics.h>
#include <framework/model.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>
#include <framework/vector.h>

namespace fw {

/**
 * This is a specialization of the scenegraph node used by models. It basically just contains a bit of extra info
 * that we want to keep around to make loading/saving them easier.
 */
class model_node: public sg::node {
private:
  model *_model;
  fw::Color _color;

protected:
  /* Renders the model node. */
  virtual void render(sg::scenegraph *sg, fw::Matrix const &model_matrix = fw::identity());

  /** Called by clone() to populate the clone. */
  virtual void populate_clone(std::shared_ptr<sg::node> clone);
public:
  model_node();
  virtual ~model_node();

  /** The index into the array of meshes that we got our data from. */
  int mesh_index;

  /** The name of this node, used to reference this node in other parts of the model, usually (e.g. in bones). */
  std::string node_name;

  /** The transform used to move the model into position in the world. */
  fw::Matrix transform;

  void set_color(fw::Color color);

  /** You can call this after setting mesh_index to set up the node. */
  void initialize(model *mdl);

  virtual std::shared_ptr<sg::node> clone();
};

}
