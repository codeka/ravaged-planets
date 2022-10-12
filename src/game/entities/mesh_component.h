#pragma once

#include <framework/color.h>
#include <framework/graphics.h>
#include <framework/model.h>
#include <framework/model_node.h>

#include <game/entities/entity.h>

namespace ent {
class OwnableComponent;

// the mesh component is a member of all entities that have mesh data, basically
// all the visible entities
class MeshComponent: public EntityComponent {
private:
  std::string model_name_;
  std::shared_ptr<fw::Model> model_;

  // The scenegraph node representing this entity.
  std::shared_ptr<fw::ModelNode> sg_node_;

public:
  static const int identifier = 200;

  MeshComponent();
  MeshComponent(std::shared_ptr<fw::Model> const &model);
  virtual ~MeshComponent();

  void apply_template(fw::lua::Value tmpl) override;

  // this is called after the Entity has added all of it's components
  virtual void initialize();

  // Gets the scenegraph node this mesh component holds. Must be called on the render thread.
  inline std::shared_ptr<fw::ModelNode> get_sg_node() const {
    FW_ENSURE_RENDER_THREAD();
    return sg_node_;
  }

  inline std::shared_ptr<fw::Model> get_model() const {
    return model_;
  }
  inline std::string get_model_name() const {
    return model_name_;
  }

  virtual int get_identifier() {
    return identifier;
  }
};

}
