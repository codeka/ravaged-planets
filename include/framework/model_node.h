#pragma once

#include <framework/scenegraph.h>

namespace fw {
class model_mesh;
class graphics;
class shader;

/**
 * This is a specialization of the scenegraph node used by models. It basically just contains a bit of extra info
 * that we want to keep around to make loading/saving them easier.
 */
class model_node: public sg::node {
protected:
  /* Renders the model node. */
  virtual void render(sg::scenegraph *sg);

  /** Called by clone() to populate the clone. */
  virtual void populate_clone(std::shared_ptr<sg::node> clone);
public:
  model_node();
  virtual ~model_node();

  /** The index into the array of meshes that we got our data from. */
  int mesh_index;

  /** The name of this node, used to reference this node in other parts of the model, usually (e.g. in bones). */
  std::string node_name;

  /** Whether or not to render in wireframe. */
  bool wireframe;

  /** The colour we'll use to draw transparents parts of the texture. */
  fw::colour colour;

  /** The transform used to move the model into position in the world. */
  fw::matrix transform;

  /** You can call this after setting mesh_index to set up the node. */
  void initialize(model &mdl);

  virtual std::shared_ptr<sg::node> clone();
};

}
