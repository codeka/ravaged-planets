#pragma once

#include <framework/scenegraph.h>
#include <framework/vector.h>

namespace fw {
class model;
class model_mesh;
class graphics;
class shader;

/**
 * This is a specialization of the scenegraph node used by models. It basically just contains a bit of extra info
 * that we want to keep around to make loading/saving them easier.
 */
class model_node: public sg::node {
private:
  model *_model;
  fw::colour _colour;

protected:
  /* Renders the model node. */
  virtual void render(sg::scenegraph *sg, fw::matrix const &model_matrix = fw::identity());

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
  fw::matrix transform;

  void set_colour(fw::colour colour);

  /** You can call this after setting mesh_index to set up the node. */
  void initialize(model *mdl);

  virtual std::shared_ptr<sg::node> clone();
};

}
