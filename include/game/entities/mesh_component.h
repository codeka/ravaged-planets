#pragma once

#include <game/entities/entity.h>
#include <framework/colour.h>

namespace fw {
class model;
}

namespace ent {
class ownable_component;

// the mesh component is a member of all entities that have mesh data, basically
// all the visible entities
class mesh_component: public entity_component {
private:
  fw::colour _colour;
  std::shared_ptr<fw::model> _model;

  // this is called when the entity's owner changes
  void owner_changed(ownable_component *ownable);

public:
  static const int identifier = 200;

  mesh_component();
  mesh_component(std::shared_ptr<fw::model> const &model);
  virtual ~mesh_component();

  // loads the given property, which we got from the entity definition file.
  virtual void apply_template(std::shared_ptr<entity_component_template> comp_template);

  // this is called after the entity has added all of it's components
  virtual void initialize();

  void set_model(std::shared_ptr<fw::model> const &model);
  std::shared_ptr<fw::model> get_model() const {
    return _model;
  }

  virtual void render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform);

  virtual int get_identifier() {
    return identifier;
  }
};

}

