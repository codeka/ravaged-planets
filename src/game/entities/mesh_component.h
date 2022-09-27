#pragma once

#include <game/entities/entity.h>
#include <framework/color.h>

namespace fw {
class Model;
}

namespace ent {
class OwnableComponent;

// the mesh component is a member of all entities that have mesh data, basically
// all the visible entities
class MeshComponent: public EntityComponent {
private:
  OwnableComponent *ownable_component_;
  std::string model_name_;
  std::shared_ptr<fw::Model> model_;

public:
  static const int identifier = 200;

  MeshComponent();
  MeshComponent(std::shared_ptr<fw::Model> const &Model);
  virtual ~MeshComponent();

  void apply_template(fw::lua::Value tmpl) override;

  // this is called after the Entity has added all of it's components
  virtual void initialize();

  std::shared_ptr<fw::Model> get_model() const {
    return model_;
  }
  std::string get_model_name() const {
    return model_name_;
  }

  virtual void render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &transform);

  virtual int get_identifier() {
    return identifier;
  }
};

}
